#include "dslvm.hpp"
#include "render.hpp"
#include "keyframes.hpp"
#include "schema_embedded.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <vector>

namespace {
using dslvm::Args; using dslvm::Value;
struct Host {
  using Keyframe=lmcad::Keyframe;
  std::vector<std::unique_ptr<std::vector<Keyframe>>> keyframes;
  std::vector<std::unique_ptr<lmcad::Material>> materials;
  std::vector<std::unique_ptr<lmcad::Shape>> shapes;
  lmcad::Camera camera;
  lmcad::Light light;
  lmcad::Color background{.08,.09,.12};
  bool has_camera=false, has_light=false;
  template<class T> Value hold(std::vector<std::unique_ptr<T>>& arena,const char* type,T value){if(arena.size()>=4096)throw dslvm::Error(std::string(type)+" object limit exceeded (4096)");arena.push_back(std::make_unique<T>(std::move(value)));return Value::object(type,arena.back().get());}
};
template<class T> T* self(void* p){return static_cast<T*>(p);}
void finite(double x,const char* name){if(!std::isfinite(x))throw dslvm::Error(std::string(name)+" must be finite");}
void positive(double x,const char* name){finite(x,name);if(x<=0)throw dslvm::Error(std::string(name)+" must be positive");}
void color_range(double x,const char* name){finite(x,name);if(x<0||x>1)throw dslvm::Error(std::string(name)+" must be between 0 and 1");}
lmcad::Vec3 vec(const Args& a,size_t i){return {a[i].f,a[i+1].f,a[i+2].f};}
lmcad::Shape make_shape(lmcad::Mesh mesh,const lmcad::Material& material){lmcad::Shape s;s.mesh=std::move(mesh);s.material=material;return s;}
void checked_method(dslvm::Registry& registry,const char* type,const char* name,
                    std::vector<std::string> params,std::string returns,dslvm::Method callback){
  registry.method(type,name,params,returns,[params,callback,type,name](void* object,Args& args){
    for(size_t i=0;i<params.size();i++){
      const auto& expected=params[i];const auto& value=args[i];
      bool valid=expected=="float"?value.kind==Value::FLOAT:
        expected=="int"?value.kind==Value::INT:expected=="string"?value.kind==Value::STR:
        expected=="bool"?value.kind==Value::BOOL:
        value.kind==Value::OBJ&&value.type==expected&&value.obj;
      if(!valid)throw dslvm::Error(std::string(type)+"."+name+" argument "+std::to_string(i+1)+" must be "+expected);
    }
    return callback(object,args);
  });
}
void checked_function(dslvm::Registry& r,const char* name,std::vector<std::string> params,std::string returns,dslvm::Function fn){
  r.function(name,params,returns,[params,fn,name](Args& args){
    if(args.size()!=params.size())throw dslvm::Error(std::string(name)+" argument count mismatch");
    for(size_t i=0;i<params.size();++i){const auto& v=args[i];const auto& e=params[i];bool ok=e=="float"?v.kind==Value::FLOAT:e=="int"?v.kind==Value::INT:e=="string"?v.kind==Value::STR:e=="bool"?v.kind==Value::BOOL:v.kind==Value::OBJ&&v.type==e&&v.obj;if(!ok)throw dslvm::Error(std::string(name)+" argument "+std::to_string(i+1)+" must be "+e);}
    return fn(args);
  });
}
void register_host(dslvm::Registry& r,Host& h){
  r.context("scene","Scene");
  checked_function(r,"material",{},"Material",[&h](Args&){return h.hold(h.materials,"Material",lmcad::Material{});});
  checked_method(r,"Scene","box",{"float","float","float","Material"},"Shape",[&h](void*,Args& a){for(int i=0;i<3;i++)positive(a[i].f,"box dimension");return h.hold(h.shapes,"Shape",make_shape(lmcad::make_box(a[0].f,a[1].f,a[2].f),*static_cast<lmcad::Material*>(a[3].obj)));});
  checked_method(r,"Scene","sphere",{"float","Material"},"Shape",[&h](void*,Args& a){positive(a[0].f,"sphere radius");return h.hold(h.shapes,"Shape",make_shape(lmcad::make_sphere(a[0].f),*static_cast<lmcad::Material*>(a[1].obj)));});
  checked_method(r,"Scene","cylinder",{"float","float","Material"},"Shape",[&h](void*,Args& a){positive(a[0].f,"cylinder radius");positive(a[1].f,"cylinder height");return h.hold(h.shapes,"Shape",make_shape(lmcad::make_cylinder(a[0].f,a[1].f),*static_cast<lmcad::Material*>(a[2].obj)));});
  checked_method(r,"Scene","camera",{"float","float","float","float","float","float","float"},"Camera",[&h](void*,Args& a){for(auto& x:a)finite(x.f,"camera argument");if(a[0].f<=0||a[0].f>=179)throw dslvm::Error("camera field of view must be between 0 and 179 degrees");h.camera={a[0].f,vec(a,1),vec(a,4),.01,10000};try{lmcad::normalized(h.camera.target-h.camera.eye);lmcad::normalized(lmcad::cross(h.camera.target-h.camera.eye,{0,1,0}));}catch(...){throw dslvm::Error("camera eye and target must differ and view direction must not be vertical");}h.has_camera=true;return Value::object("Camera",&h.camera);});
  checked_method(r,"Scene","directional_light",{"float","float","float","float"},"Light",[&h](void*,Args& a){for(auto& x:a)finite(x.f,"directional light argument");if(a[3].f<0)throw dslvm::Error("light intensity must not be negative");try{h.light={lmcad::normalized(vec(a,0)),a[3].f};}catch(...){throw dslvm::Error("light direction must be nonzero");}h.has_light=true;return Value::object("Light",&h.light);});
  auto values=[&h](){std::vector<lmcad::Shape> s;s.reserve(h.shapes.size());for(const auto& p:h.shapes)s.push_back(*p);return s;};
  auto render=[&h,values](const std::string& path,int64_t w,int64_t height,bool png){if(!h.has_camera)throw dslvm::Error("render requires a camera");if(path.empty())throw dslvm::Error("output path must not be empty");if(w<1||height<1||w>8192||height>8192||uint64_t(w)*height>16777216)throw dslvm::Error("image dimensions exceed limits (1..8192 per side, 16777216 pixels total)");try{auto scene=values();auto image=lmcad::render(scene,h.camera,h.light,h.background,w,height);if(png)lmcad::write_png(path,image);else lmcad::write_ppm(path,image);}catch(const dslvm::Error&){throw;}catch(const std::exception& e){throw dslvm::Error(e.what());}};
  checked_method(r,"Scene","render_png",{"string","int","int"},"void",[render](void*,Args& a){render(a[0].s,a[1].i,a[2].i,true);return Value::none();});
  checked_method(r,"Scene","render_ppm",{"string","int","int"},"void",[render](void*,Args& a){render(a[0].s,a[1].i,a[2].i,false);return Value::none();});
  checked_method(r,"Scene","export_stl",{"string"},"void",[&h,values](void*,Args& a){if(a[0].s.empty())throw dslvm::Error("output path must not be empty");try{auto scene=values();lmcad::write_stl(a[0].s,scene);}catch(const std::exception& e){throw dslvm::Error(e.what());}return Value::none();});
  checked_method(r,"Scene","export_obj",{"string"},"void",[&h,values](void*,Args& a){if(a[0].s.empty())throw dslvm::Error("output path must not be empty");try{auto scene=values();lmcad::write_obj(a[0].s,scene);}catch(const std::exception& e){throw dslvm::Error(e.what());}return Value::none();});
  checked_method(r,"Scene","background",{"float","float","float"},"Scene",[&h](void* p,Args& a){for(int i=0;i<3;i++)color_range(a[i].f,"background channel");h.background={a[0].f,a[1].f,a[2].f};return Value::object("Scene",p);});
  checked_method(r,"Shape","translate",{"float","float","float"},"Shape",[](void* p,Args& a){for(auto& x:a)finite(x.f,"translation");auto next=self<lmcad::Shape>(p)->translation+vec(a,0);for(double x:{next.x,next.y,next.z})finite(x,"accumulated translation");self<lmcad::Shape>(p)->translation=next;return Value::object("Shape",p);});
  checked_method(r,"Shape","rotate",{"float","float","float"},"Shape",[](void* p,Args& a){for(auto& x:a)finite(x.f,"rotation");auto next=self<lmcad::Shape>(p)->rotation+vec(a,0);for(double x:{next.x,next.y,next.z})finite(x,"accumulated rotation");self<lmcad::Shape>(p)->rotation=next;return Value::object("Shape",p);});
  checked_method(r,"Shape","scale",{"float","float","float"},"Shape",[](void* p,Args& a){for(auto& x:a)positive(std::abs(x.f),"scale magnitude");auto* s=self<lmcad::Shape>(p);auto next=lmcad::Vec3{s->scale.x*a[0].f,s->scale.y*a[1].f,s->scale.z*a[2].f};for(double x:{next.x,next.y,next.z})positive(std::abs(x),"accumulated scale magnitude");s->scale=next;return Value::object("Shape",p);});
  checked_method(r,"Shape","visible",{"bool"},"Shape",[](void* p,Args& a){self<lmcad::Shape>(p)->visible=a[0].b;return Value::object("Shape",p);});
  checked_method(r,"Shape","name",{"string"},"Shape",[](void* p,Args& a){if(a[0].s.size()>1024)throw dslvm::Error("shape name exceeds 1024 bytes");self<lmcad::Shape>(p)->name=a[0].s;return Value::object("Shape",p);});
  checked_method(r,"Shape","position",{"float","float","float"},"Shape",[](void* p,Args& a){for(auto& x:a)finite(x.f,"position");auto* s=self<lmcad::Shape>(p);s->translation=vec(a,0);return Value::object("Shape",p);});
  checked_method(r,"Shape","color",{"float","float","float"},"Shape",[](void* p,Args& a){for(auto& x:a)color_range(x.f,"shape color");auto* s=self<lmcad::Shape>(p);s->material.color={a[0].f,a[1].f,a[2].f};return Value::object("Shape",p);});
  checked_method(r,"Camera","look_at",{"float","float","float","float","float","float"},"Camera",[&h](void* p,Args& a){for(auto& x:a)finite(x.f,"camera look_at argument");auto* c=self<lmcad::Camera>(p);c->eye=vec(a,0);c->target=vec(a,3);try{lmcad::normalized(c->target-c->eye);lmcad::normalized(lmcad::cross(c->target-c->eye,{0,1,0}));}catch(...){throw dslvm::Error("camera eye and target must differ and view direction must not be vertical");}return Value::object("Camera",p);});
  checked_method(r,"Camera","clipping",{"float","float"},"Camera",[](void* p,Args& a){positive(a[0].f,"near clipping plane");positive(a[1].f,"far clipping plane");if(a[1].f<=a[0].f)throw dslvm::Error("far clipping plane must be greater than near clipping plane");auto* c=self<lmcad::Camera>(p);c->near_clip=a[0].f;c->far_clip=a[1].f;return Value::object("Camera",p);});
  checked_method(r,"Material","color",{"float","float","float"},"Material",[](void* p,Args& a){for(int i=0;i<3;i++)color_range(a[i].f,"material color channel");self<lmcad::Material>(p)->color={a[0].f,a[1].f,a[2].f};return Value::object("Material",p);});
  checked_method(r,"Material","roughness",{"float"},"Material",[](void* p,Args& a){color_range(a[0].f,"roughness");self<lmcad::Material>(p)->roughness=a[0].f;return Value::object("Material",p);});
  checked_method(r,"Light","intensity",{"float"},"Light",[](void* p,Args& a){finite(a[0].f,"light intensity");if(a[0].f<0)throw dslvm::Error("light intensity must not be negative");self<lmcad::Light>(p)->intensity=a[0].f;return Value::object("Light",p);});
  checked_function(r,"read_keyframes_csv",{"string"},"Keyframes",[&h](Args& a){
    auto rows=std::make_unique<std::vector<Host::Keyframe>>(lmcad::read_keyframes_csv(a[0].s));
    return h.hold(h.keyframes,"Keyframes",std::move(*rows));
  });
  checked_method(r,"Keyframes","len",{},"int",[](void* p,Args&){return Value::integer(static_cast<int64_t>(self<std::vector<Host::Keyframe>>(p)->size()));});
  checked_method(r,"Keyframes","at",{"int"},"Keyframe",[&h](void* p,Args& a){auto& rows=*self<std::vector<Host::Keyframe>>(p);if(a[0].i<0||static_cast<uint64_t>(a[0].i)>=rows.size())throw dslvm::Error("keyframe index out of range");return Value::object("Keyframe",&rows[static_cast<size_t>(a[0].i)]);});
  for(const char* field:{"time","x","y","z","r","g","b"})checked_method(r,"Keyframe",field,{},"float",[field](void* p,Args&){auto* k=self<Host::Keyframe>(p);double v=std::string(field)=="time"?k->t:std::string(field)=="x"?k->x:std::string(field)=="y"?k->y:std::string(field)=="z"?k->z:std::string(field)=="r"?k->r:std::string(field)=="g"?k->g:k->b;return Value::real(v);});
}
}
int main(int argc,char** argv){
  if(argc==2&&std::string(argv[1])=="--schema") {
    std::cout.write(reinterpret_cast<const char*>(lmcad::embedded_schema),
                    static_cast<std::streamsize>(lmcad::embedded_schema_size));
    return std::cout ? 0 : 4;
  }
  Host host;
  dslvm::Registry reg;
  register_host(reg,host);
  if(argc==2&&std::string(argv[1])=="--dsl-signatures") {
    for(const auto& signature:reg.signatures()) std::cout<<signature<<"\n";
    return 0;
  }
  if(argc<3||std::string(argv[1])!="--dsl") {
    std::cerr<<"usage: lmcad --schema | --dsl-signatures | --dsl FILE [FILE ...]\n";
    return 2;
  }
  // Validate the complete invocation before any block can write output.
  for(int i=2;i<argc;i++) {
    if(std::string(argv[i]).empty()||std::string(argv[i]).rfind("--",0)==0) {
      std::cerr<<"usage: lmcad --schema | --dsl-signatures | --dsl FILE [FILE ...]\n";
      return 2;
    }
  }
  const std::string schema(reinterpret_cast<const char*>(lmcad::embedded_schema),
                           lmcad::embedded_schema_size);
  dslvm::Context context;
  context.set("scene",&host);
  for(int i=2;i<argc;i++) {
    dslvm::Program program;
    try {
      program=dslvm::Program::load(dslvm::read_file(argv[i]),schema,reg);
    } catch(const dslvm::Error& error) {
      return dslvm::report_error("load",error);
    }
    try {
      program.run(context,reg);
    } catch(const dslvm::Error& error) {
      return dslvm::report_error("run",error);
    }
    dslvm::report_ok();
  }
  return 0;
}
