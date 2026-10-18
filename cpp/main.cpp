#include "dslvm.hpp"
#include "render.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

namespace {
using dslvm::Args; using dslvm::Value;
struct Host {
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
                    std::vector<std::string> params,dslvm::Method callback){
  auto arity=params.size();
  registry.method(type,name,arity,[params=std::move(params),callback,type,name](void* object,Args& args){
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
void register_host(dslvm::Registry& r,Host& h){
  r.context("scene","Scene");
  r.function("material",0,[&h](Args&){return h.hold(h.materials,"Material",lmcad::Material{});});
  checked_method(r,"Scene","box",{"float","float","float","Material"},[&h](void*,Args& a){for(int i=0;i<3;i++)positive(a[i].f,"box dimension");return h.hold(h.shapes,"Shape",make_shape(lmcad::make_box(a[0].f,a[1].f,a[2].f),*static_cast<lmcad::Material*>(a[3].obj)));});
  checked_method(r,"Scene","sphere",{"float","Material"},[&h](void*,Args& a){positive(a[0].f,"sphere radius");return h.hold(h.shapes,"Shape",make_shape(lmcad::make_sphere(a[0].f),*static_cast<lmcad::Material*>(a[1].obj)));});
  checked_method(r,"Scene","cylinder",{"float","float","Material"},[&h](void*,Args& a){positive(a[0].f,"cylinder radius");positive(a[1].f,"cylinder height");return h.hold(h.shapes,"Shape",make_shape(lmcad::make_cylinder(a[0].f,a[1].f),*static_cast<lmcad::Material*>(a[2].obj)));});
  checked_method(r,"Scene","camera",{"float","float","float","float","float","float","float"},[&h](void*,Args& a){for(auto& x:a)finite(x.f,"camera argument");if(a[0].f<=0||a[0].f>=179)throw dslvm::Error("camera field of view must be between 0 and 179 degrees");h.camera={a[0].f,vec(a,1),vec(a,4),.01,10000};try{lmcad::normalized(h.camera.target-h.camera.eye);lmcad::normalized(lmcad::cross(h.camera.target-h.camera.eye,{0,1,0}));}catch(...){throw dslvm::Error("camera eye and target must differ and view direction must not be vertical");}h.has_camera=true;return Value::object("Camera",&h.camera);});
  checked_method(r,"Scene","directional_light",{"float","float","float","float"},[&h](void*,Args& a){for(auto& x:a)finite(x.f,"directional light argument");if(a[3].f<0)throw dslvm::Error("light intensity must not be negative");try{h.light={lmcad::normalized(vec(a,0)),a[3].f};}catch(...){throw dslvm::Error("light direction must be nonzero");}h.has_light=true;return Value::object("Light",&h.light);});
  auto values=[&h](){std::vector<lmcad::Shape> s;s.reserve(h.shapes.size());for(const auto& p:h.shapes)s.push_back(*p);return s;};
  auto render=[&h,values](const std::string& path,int w,int height,bool png){if(!h.has_camera)throw dslvm::Error("render requires a camera");if(path.empty())throw dslvm::Error("output path must not be empty");if(w<1||height<1||w>8192||height>8192||uint64_t(w)*height>16777216)throw dslvm::Error("image dimensions exceed limits (1..8192 per side, 16777216 pixels total)");try{auto scene=values();auto image=lmcad::render(scene,h.camera,h.light,h.background,w,height);if(png)lmcad::write_png(path,image);else lmcad::write_ppm(path,image);}catch(const dslvm::Error&){throw;}catch(const std::exception& e){throw dslvm::Error(e.what());}};
  checked_method(r,"Scene","render_png",{"string","int","int"},[render](void*,Args& a){render(a[0].s,a[1].i,a[2].i,true);return Value::none();});
  checked_method(r,"Scene","render_ppm",{"string","int","int"},[render](void*,Args& a){render(a[0].s,a[1].i,a[2].i,false);return Value::none();});
  checked_method(r,"Scene","export_stl",{"string"},[&h,values](void*,Args& a){if(a[0].s.empty())throw dslvm::Error("output path must not be empty");try{auto scene=values();lmcad::write_stl(a[0].s,scene);}catch(const std::exception& e){throw dslvm::Error(e.what());}return Value::none();});
  checked_method(r,"Scene","export_obj",{"string"},[&h,values](void*,Args& a){if(a[0].s.empty())throw dslvm::Error("output path must not be empty");try{auto scene=values();lmcad::write_obj(a[0].s,scene);}catch(const std::exception& e){throw dslvm::Error(e.what());}return Value::none();});
  checked_method(r,"Scene","background",{"float","float","float"},[&h](void* p,Args& a){for(int i=0;i<3;i++)color_range(a[i].f,"background channel");h.background={a[0].f,a[1].f,a[2].f};return Value::object("Scene",p);});
  checked_method(r,"Shape","translate",{"float","float","float"},[](void* p,Args& a){for(auto& x:a)finite(x.f,"translation");auto next=self<lmcad::Shape>(p)->translation+vec(a,0);for(double x:{next.x,next.y,next.z})finite(x,"accumulated translation");self<lmcad::Shape>(p)->translation=next;return Value::object("Shape",p);});
  checked_method(r,"Shape","rotate",{"float","float","float"},[](void* p,Args& a){for(auto& x:a)finite(x.f,"rotation");auto next=self<lmcad::Shape>(p)->rotation+vec(a,0);for(double x:{next.x,next.y,next.z})finite(x,"accumulated rotation");self<lmcad::Shape>(p)->rotation=next;return Value::object("Shape",p);});
  checked_method(r,"Shape","scale",{"float","float","float"},[](void* p,Args& a){for(auto& x:a)positive(std::abs(x.f),"scale magnitude");auto* s=self<lmcad::Shape>(p);auto next=lmcad::Vec3{s->scale.x*a[0].f,s->scale.y*a[1].f,s->scale.z*a[2].f};for(double x:{next.x,next.y,next.z})positive(std::abs(x),"accumulated scale magnitude");s->scale=next;return Value::object("Shape",p);});
  checked_method(r,"Shape","visible",{"bool"},[](void* p,Args& a){self<lmcad::Shape>(p)->visible=a[0].b;return Value::object("Shape",p);});
  checked_method(r,"Shape","name",{"string"},[](void* p,Args& a){if(a[0].s.size()>1024)throw dslvm::Error("shape name exceeds 1024 bytes");self<lmcad::Shape>(p)->name=a[0].s;return Value::object("Shape",p);});
  checked_method(r,"Camera","look_at",{"float","float","float","float","float","float"},[&h](void* p,Args& a){for(auto& x:a)finite(x.f,"camera look_at argument");auto* c=self<lmcad::Camera>(p);c->eye=vec(a,0);c->target=vec(a,3);try{lmcad::normalized(c->target-c->eye);lmcad::normalized(lmcad::cross(c->target-c->eye,{0,1,0}));}catch(...){throw dslvm::Error("camera eye and target must differ and view direction must not be vertical");}return Value::object("Camera",p);});
  checked_method(r,"Camera","clipping",{"float","float"},[](void* p,Args& a){positive(a[0].f,"near clipping plane");positive(a[1].f,"far clipping plane");if(a[1].f<=a[0].f)throw dslvm::Error("far clipping plane must be greater than near clipping plane");auto* c=self<lmcad::Camera>(p);c->near_clip=a[0].f;c->far_clip=a[1].f;return Value::object("Camera",p);});
  checked_method(r,"Material","color",{"float","float","float"},[](void* p,Args& a){for(int i=0;i<3;i++)color_range(a[i].f,"material color channel");self<lmcad::Material>(p)->color={a[0].f,a[1].f,a[2].f};return Value::object("Material",p);});
  checked_method(r,"Material","roughness",{"float"},[](void* p,Args& a){color_range(a[0].f,"roughness");self<lmcad::Material>(p)->roughness=a[0].f;return Value::object("Material",p);});
  checked_method(r,"Light","intensity",{"float"},[](void* p,Args& a){finite(a[0].f,"light intensity");if(a[0].f<0)throw dslvm::Error("light intensity must not be negative");self<lmcad::Light>(p)->intensity=a[0].f;return Value::object("Light",p);});
}
}
int main(int argc,char** argv){
  std::string dsl,schema;
  for(int i=1;i<argc;i++){std::string a=argv[i];if(a=="--dsl"||a=="--schema"){if(i+1>=argc){std::cerr<<"error: "<<a<<" needs a value\n";return 2;}if(a=="--dsl")dsl=argv[++i];else schema=argv[++i];}else{std::cerr<<"usage: lmcad --dsl FILE --schema FILE\n";return 2;}}
  if(dsl.empty()||schema.empty()){std::cerr<<"error: --dsl FILE and --schema FILE are required\n";return 2;}
  Host host;dslvm::Registry reg;register_host(reg,host);dslvm::Program program;
  try{program=dslvm::Program::load(dslvm::read_file(dsl),dslvm::read_file(schema),reg);}catch(const dslvm::Error& e){std::cerr<<"error: "<<dsl<<": "<<e.what()<<"\n";return 1;}
  dslvm::Context ctx;ctx.set("scene",&host);try{program.run(ctx,reg);}catch(const dslvm::Error& e){std::cerr<<"dsl runtime error: "<<e.what()<<"\n";return 3;}
  return 0;
}
