#include "dslvm.hpp"
#include "render.hpp"
#include "cad.hpp"
#include "keyframes.hpp"
#include "schema_embedded.hpp"
#include <Standard_Failure.hxx>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <memory>
#include <map>
#include <sstream>
#include <vector>

namespace {
using dslvm::Args; using dslvm::Value;
struct Host {
  using Keyframe=lmcad::Keyframe;
  struct CadShape { lmcad::Shape display; TopoDS_Shape brep; };
  std::vector<std::unique_ptr<std::vector<Keyframe>>> keyframes;
  std::vector<std::unique_ptr<lmcad::Material>> materials;
  std::vector<std::unique_ptr<CadShape>> shapes;
  std::vector<std::unique_ptr<lmcad::cad::Sketch>> sketches;
  lmcad::Camera camera;
  lmcad::Light light;
  struct LightHandle { double* intensity; };
  LightHandle key_handle{&light.intensity}, fill_handle{&light.fill_intensity};
  lmcad::Color background{.08,.09,.12};
  int supersample=2;
  bool has_camera=false, has_light=false;
  template<class T> Value hold(std::vector<std::unique_ptr<T>>& arena,const char* type,T value){if(arena.size()>=4096)throw dslvm::Error(std::string(type)+" object limit exceeded (4096)");arena.push_back(std::make_unique<T>(std::move(value)));return Value::object(type,arena.back().get());}
};
template<class T> T* self(void* p){return static_cast<T*>(p);}
void finite(double x,const char* name){if(!std::isfinite(x))throw dslvm::Error(std::string(name)+" must be finite");}
void positive(double x,const char* name){finite(x,name);if(x<=0)throw dslvm::Error(std::string(name)+" must be positive");}
void color_range(double x,const char* name){finite(x,name);if(x<0||x>1)throw dslvm::Error(std::string(name)+" must be between 0 and 1");}
lmcad::Vec3 vec(const Args& a,size_t i){return {a[i].f,a[i+1].f,a[i+2].f};}
Host::CadShape make_shape(TopoDS_Shape brep,const lmcad::Material& material){Host::CadShape s;s.brep=std::move(brep);s.display.mesh=lmcad::cad::tessellate(s.brep);s.display.material=material;return s;}
TopoDS_Shape effective(Host::CadShape& s){return lmcad::cad::transform(s.brep,s.display.translation,s.display.rotation,s.display.scale);}
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
    try{return callback(object,args);}
    catch(const dslvm::Error&){throw;}
    catch(const Standard_Failure& e){throw dslvm::Error(e.what());}
    catch(const std::exception& e){throw dslvm::Error(e.what());}
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
  checked_method(r,"Scene","box",{"float","float","float","Material"},"Shape",[&h](void*,Args& a){for(int i=0;i<3;i++)positive(a[i].f,"box dimension");return h.hold(h.shapes,"Shape",make_shape(lmcad::cad::box(a[0].f,a[1].f,a[2].f),*static_cast<lmcad::Material*>(a[3].obj)));});
  checked_method(r,"Scene","sphere",{"float","Material"},"Shape",[&h](void*,Args& a){positive(a[0].f,"sphere radius");return h.hold(h.shapes,"Shape",make_shape(lmcad::cad::sphere(a[0].f),*static_cast<lmcad::Material*>(a[1].obj)));});
  checked_method(r,"Scene","cylinder",{"float","float","Material"},"Shape",[&h](void*,Args& a){positive(a[0].f,"cylinder radius");positive(a[1].f,"cylinder height");return h.hold(h.shapes,"Shape",make_shape(lmcad::cad::cylinder(a[0].f,a[1].f),*static_cast<lmcad::Material*>(a[2].obj)));});
  checked_method(r,"Scene","camera",{"float","float","float","float","float","float","float"},"Camera",[&h](void*,Args& a){for(auto& x:a)finite(x.f,"camera argument");if(a[0].f<=0||a[0].f>=179)throw dslvm::Error("camera field of view must be between 0 and 179 degrees");h.camera={a[0].f,vec(a,1),vec(a,4),.01,10000};try{lmcad::normalized(h.camera.target-h.camera.eye);lmcad::normalized(lmcad::cross(h.camera.target-h.camera.eye,{0,1,0}));}catch(...){throw dslvm::Error("camera eye and target must differ and view direction must not be vertical");}h.has_camera=true;return Value::object("Camera",&h.camera);});
  checked_method(r,"Scene","directional_light",{"float","float","float","float"},"Light",[&h](void*,Args& a){for(auto& x:a)finite(x.f,"directional light argument");if(a[3].f<0)throw dslvm::Error("light intensity must not be negative");try{h.light.direction=lmcad::normalized(vec(a,0));}catch(...){throw dslvm::Error("light direction must be nonzero");}h.light.intensity=a[3].f;h.has_light=true;return Value::object("Light",&h.key_handle);});
  checked_method(r,"Scene","fill_light",{"float","float","float","float"},"Light",[&h](void*,Args& a){for(auto& x:a)finite(x.f,"fill light argument");if(a[3].f<0)throw dslvm::Error("fill light intensity must not be negative");try{h.light.fill_direction=lmcad::normalized(vec(a,0));}catch(...){throw dslvm::Error("fill light direction must be nonzero");}h.light.fill_intensity=a[3].f;return Value::object("Light",&h.fill_handle);});
  checked_method(r,"Scene","ambient",{"float"},"Scene",[&h](void* p,Args& a){color_range(a[0].f,"ambient level");h.light.ambient=a[0].f;return Value::object("Scene",p);});
  checked_method(r,"Scene","supersample",{"int"},"Scene",[&h](void* p,Args& a){if(a[0].i<1||a[0].i>4)throw dslvm::Error("supersample must be between 1 and 4");h.supersample=static_cast<int>(a[0].i);return Value::object("Scene",p);});
  auto values=[&h](){std::vector<lmcad::Shape> s;s.reserve(h.shapes.size());for(const auto& p:h.shapes)s.push_back(p->display);return s;};
  auto render=[&h,values](const std::string& path,int64_t w,int64_t height,bool png){if(!h.has_camera)throw dslvm::Error("render requires a camera");if(path.empty())throw dslvm::Error("output path must not be empty");if(w<1||height<1||w>8192||height>8192||uint64_t(w)*height>16777216)throw dslvm::Error("image dimensions exceed limits (1..8192 per side, 16777216 pixels total)");try{auto scene=values();auto image=lmcad::render(scene,h.camera,h.light,h.background,w,height,h.supersample);if(png)lmcad::write_png(path,image);else lmcad::write_ppm(path,image);}catch(const dslvm::Error&){throw;}catch(const std::exception& e){throw dslvm::Error(e.what());}};
  checked_method(r,"Scene","render_png",{"string","int","int"},"void",[render](void*,Args& a){render(a[0].s,a[1].i,a[2].i,true);return Value::none();});
  checked_method(r,"Scene","render_ppm",{"string","int","int"},"void",[render](void*,Args& a){render(a[0].s,a[1].i,a[2].i,false);return Value::none();});
  checked_method(r,"Scene","export_stl",{"string"},"void",[&h,values](void*,Args& a){if(a[0].s.empty())throw dslvm::Error("output path must not be empty");try{auto scene=values();lmcad::write_stl(a[0].s,scene);}catch(const std::exception& e){throw dslvm::Error(e.what());}return Value::none();});
  checked_method(r,"Scene","export_obj",{"string"},"void",[&h,values](void*,Args& a){if(a[0].s.empty())throw dslvm::Error("output path must not be empty");try{auto scene=values();lmcad::write_obj(a[0].s,scene);}catch(const std::exception& e){throw dslvm::Error(e.what());}return Value::none();});
  checked_method(r,"Scene","background",{"float","float","float"},"Scene",[&h](void* p,Args& a){for(int i=0;i<3;i++)color_range(a[i].f,"background channel");h.background={a[0].f,a[1].f,a[2].f};return Value::object("Scene",p);});
  checked_method(r,"Scene","sketch",{},"Sketch",[&h](void*,Args&){return h.hold(h.sketches,"Sketch",lmcad::cad::Sketch{});});
  checked_method(r,"Sketch","point",{"float","float"},"Sketch",[](void* p,Args& a){for(auto& v:a)finite(v.f,"sketch coordinate");auto* sk=self<lmcad::cad::Sketch>(p);if(sk->closed)throw dslvm::Error("cannot add points after sketch.close()");if(sk->points.size()>=10000)throw dslvm::Error("sketch point limit exceeded");sk->points.emplace_back(a[0].f,a[1].f);return Value::object("Sketch",p);});
  checked_method(r,"Sketch","close",{},"Sketch",[](void* p,Args&){auto* sk=self<lmcad::cad::Sketch>(p);if(sk->points.size()<3)throw dslvm::Error("sketch needs at least three points");sk->closed=true;return Value::object("Sketch",p);});
  checked_method(r,"Scene","extrude",{"Sketch","float","Material"},"Shape",[&h](void*,Args& a){positive(a[1].f,"extrusion height");return h.hold(h.shapes,"Shape",make_shape(lmcad::cad::extrude(*self<lmcad::cad::Sketch>(a[0].obj),a[1].f),*static_cast<lmcad::Material*>(a[2].obj)));});
  checked_method(r,"Scene","revolve_y",{"Sketch","Material"},"Shape",[&h](void*,Args& a){return h.hold(h.shapes,"Shape",make_shape(lmcad::cad::revolve_y(*self<lmcad::cad::Sketch>(a[0].obj)),*static_cast<lmcad::Material*>(a[1].obj)));});
  checked_method(r,"Scene","fit_camera",{"float","float"},"Camera",[&h](void*,Args& a){finite(a[0].f,"camera field of view");positive(a[1].f,"camera fit margin");if(a[0].f<=1||a[0].f>=179)throw dslvm::Error("camera field of view must be between 1 and 179 degrees");if(a[1].f<1)throw dslvm::Error("camera fit margin must be at least 1");double lo[3]={INFINITY,INFINITY,INFINITY},hi[3]={-INFINITY,-INFINITY,-INFINITY};bool any=false;for(auto& obj:h.shapes)if(obj->display.visible){any=true;auto b=lmcad::cad::bounds(effective(*obj));for(int i=0;i<3;i++){lo[i]=std::min(lo[i],b[i]);hi[i]=std::max(hi[i],b[i+3]);}}if(!any)throw dslvm::Error("fit_camera requires at least one visible shape");lmcad::Vec3 center{(lo[0]+hi[0])/2,(lo[1]+hi[1])/2,(lo[2]+hi[2])/2};double radius=lmcad::length({(hi[0]-lo[0])/2,(hi[1]-lo[1])/2,(hi[2]-lo[2])/2});double dist=std::max(1.0,radius/std::sin(a[0].f*std::acos(-1.0)/360.0)*a[1].f);auto dir=lmcad::normalized({1.4,1.1,1.7});h.camera={a[0].f,center+dir*dist,center,.01,dist+radius*4};h.has_camera=true;return Value::object("Camera",&h.camera);});
  checked_method(r,"Scene","assert_close",{"float","float","float"},"void",[](void*,Args& a){for(auto& v:a)finite(v.f,"assert_close argument");if(a[2].f<0||std::abs(a[0].f-a[1].f)>a[2].f){std::ostringstream message;message<<"assert_close failed: actual="<<std::setprecision(17)<<a[0].f<<", expected="<<a[1].f;throw dslvm::Error(message.str());}return Value::none();});
  auto bake=[](Host::CadShape* sh,TopoDS_Shape shape){sh->brep=std::move(shape);sh->display.mesh=lmcad::cad::tessellate(sh->brep);sh->display.translation={};sh->display.rotation={};sh->display.scale={1,1,1};};
  checked_method(r,"Shape","union",{"Shape"},"Shape",[bake](void* p,Args& a){auto* sh=self<Host::CadShape>(p);bake(sh,lmcad::cad::boolean_union(effective(*sh),effective(*self<Host::CadShape>(a[0].obj))));return Value::object("Shape",p);});
  checked_method(r,"Shape","cut",{"Shape"},"Shape",[bake](void* p,Args& a){auto* sh=self<Host::CadShape>(p);bake(sh,lmcad::cad::cut(effective(*sh),effective(*self<Host::CadShape>(a[0].obj))));return Value::object("Shape",p);});
  checked_method(r,"Shape","intersect",{"Shape"},"Shape",[bake](void* p,Args& a){auto* sh=self<Host::CadShape>(p);bake(sh,lmcad::cad::intersect(effective(*sh),effective(*self<Host::CadShape>(a[0].obj))));return Value::object("Shape",p);});
  checked_method(r,"Shape","fillet",{"float"},"Shape",[bake](void* p,Args& a){positive(a[0].f,"fillet radius");auto* sh=self<Host::CadShape>(p);bake(sh,lmcad::cad::fillet(effective(*sh),a[0].f));return Value::object("Shape",p);});
  checked_method(r,"Shape","chamfer",{"float"},"Shape",[bake](void* p,Args& a){positive(a[0].f,"chamfer distance");auto* sh=self<Host::CadShape>(p);bake(sh,lmcad::cad::chamfer(effective(*sh),a[0].f));return Value::object("Shape",p);});
  for(const char* method:{"hollow","hollow_top"})checked_method(r,"Shape",method,{"float"},"Shape",[bake,method](void* p,Args& a){positive(a[0].f,"hollow thickness");auto* sh=self<Host::CadShape>(p);bake(sh,lmcad::cad::hollow(effective(*sh),a[0].f,std::string(method)=="hollow_top"));return Value::object("Shape",p);});
  checked_method(r,"Shape","mirror",{"string"},"Shape",[bake](void* p,Args& a){if(a[0].s!="x"&&a[0].s!="y"&&a[0].s!="z")throw dslvm::Error("mirror axis must be x, y, or z");auto* sh=self<Host::CadShape>(p);bake(sh,lmcad::cad::mirror(effective(*sh),a[0].s[0]));return Value::object("Shape",p);});
  checked_method(r,"Shape","volume",{},"float",[](void* p,Args&){return Value::real(lmcad::cad::volume(effective(*self<Host::CadShape>(p))));});
  for(const char* field:{"min_x","min_y","min_z","max_x","max_y","max_z"})checked_method(r,"Shape",field,{},"float",[field](void* p,Args&){auto b=lmcad::cad::bounds(effective(*self<Host::CadShape>(p)));size_t i=std::string(field)=="min_x"?0:std::string(field)=="min_y"?1:std::string(field)=="min_z"?2:std::string(field)=="max_x"?3:std::string(field)=="max_y"?4:5;return Value::real(b[i]);});
  checked_method(r,"Shape","translate",{"float","float","float"},"Shape",[](void* p,Args& a){for(auto& x:a)finite(x.f,"translation");auto next=self<Host::CadShape>(p)->display.translation+vec(a,0);for(double x:{next.x,next.y,next.z})finite(x,"accumulated translation");self<Host::CadShape>(p)->display.translation=next;return Value::object("Shape",p);});
  checked_method(r,"Shape","rotate",{"float","float","float"},"Shape",[](void* p,Args& a){for(auto& x:a)finite(x.f,"rotation");auto next=self<Host::CadShape>(p)->display.rotation+vec(a,0);for(double x:{next.x,next.y,next.z})finite(x,"accumulated rotation");self<Host::CadShape>(p)->display.rotation=next;return Value::object("Shape",p);});
  checked_method(r,"Shape","scale",{"float","float","float"},"Shape",[](void* p,Args& a){for(auto& x:a)positive(std::abs(x.f),"scale magnitude");auto* s=self<lmcad::Shape>(p);auto next=lmcad::Vec3{s->scale.x*a[0].f,s->scale.y*a[1].f,s->scale.z*a[2].f};for(double x:{next.x,next.y,next.z})positive(std::abs(x),"accumulated scale magnitude");s->scale=next;return Value::object("Shape",p);});
  checked_method(r,"Shape","visible",{"bool"},"Shape",[](void* p,Args& a){self<Host::CadShape>(p)->display.visible=a[0].b;return Value::object("Shape",p);});
  checked_method(r,"Shape","name",{"string"},"Shape",[](void* p,Args& a){if(a[0].s.size()>1024)throw dslvm::Error("shape name exceeds 1024 bytes");self<Host::CadShape>(p)->display.name=a[0].s;return Value::object("Shape",p);});
  checked_method(r,"Shape","position",{"float","float","float"},"Shape",[](void* p,Args& a){for(auto& x:a)finite(x.f,"position");auto* s=self<lmcad::Shape>(p);s->translation=vec(a,0);return Value::object("Shape",p);});
  checked_method(r,"Shape","color",{"float","float","float"},"Shape",[](void* p,Args& a){for(auto& x:a)color_range(x.f,"shape color");auto* s=self<lmcad::Shape>(p);s->material.color={a[0].f,a[1].f,a[2].f};return Value::object("Shape",p);});
  checked_method(r,"Camera","look_at",{"float","float","float","float","float","float"},"Camera",[&h](void* p,Args& a){for(auto& x:a)finite(x.f,"camera look_at argument");auto* c=self<lmcad::Camera>(p);c->eye=vec(a,0);c->target=vec(a,3);try{lmcad::normalized(c->target-c->eye);lmcad::normalized(lmcad::cross(c->target-c->eye,{0,1,0}));}catch(...){throw dslvm::Error("camera eye and target must differ and view direction must not be vertical");}return Value::object("Camera",p);});
  checked_method(r,"Camera","clipping",{"float","float"},"Camera",[](void* p,Args& a){positive(a[0].f,"near clipping plane");positive(a[1].f,"far clipping plane");if(a[1].f<=a[0].f)throw dslvm::Error("far clipping plane must be greater than near clipping plane");auto* c=self<lmcad::Camera>(p);c->near_clip=a[0].f;c->far_clip=a[1].f;return Value::object("Camera",p);});
  checked_method(r,"Material","color",{"float","float","float"},"Material",[](void* p,Args& a){for(int i=0;i<3;i++)color_range(a[i].f,"material color channel");self<lmcad::Material>(p)->color={a[0].f,a[1].f,a[2].f};return Value::object("Material",p);});
  checked_method(r,"Material","roughness",{"float"},"Material",[](void* p,Args& a){color_range(a[0].f,"roughness");self<lmcad::Material>(p)->roughness=a[0].f;return Value::object("Material",p);});
  checked_method(r,"Light","intensity",{"float"},"Light",[](void* p,Args& a){finite(a[0].f,"light intensity");if(a[0].f<0)throw dslvm::Error("light intensity must not be negative");*self<Host::LightHandle>(p)->intensity=a[0].f;return Value::object("Light",p);});
  checked_function(r,"read_keyframes_csv",{"string"},"Keyframes",[&h](Args& a){
    auto rows=std::make_unique<std::vector<Host::Keyframe>>(lmcad::read_keyframes_csv(a[0].s));
    return h.hold(h.keyframes,"Keyframes",std::move(*rows));
  });
  checked_method(r,"Keyframes","len",{},"int",[](void* p,Args&){return Value::integer(static_cast<int64_t>(self<std::vector<Host::Keyframe>>(p)->size()));});
  checked_method(r,"Keyframes","at",{"int"},"Keyframe",[&h](void* p,Args& a){auto& rows=*self<std::vector<Host::Keyframe>>(p);if(a[0].i<0||static_cast<uint64_t>(a[0].i)>=rows.size())throw dslvm::Error("keyframe index out of range");return Value::object("Keyframe",&rows[static_cast<size_t>(a[0].i)]);});
  for(const char* field:{"time","x","y","z","r","g","b"})checked_method(r,"Keyframe",field,{},"float",[field](void* p,Args&){auto* k=self<Host::Keyframe>(p);double v=std::string(field)=="time"?k->t:std::string(field)=="x"?k->x:std::string(field)=="y"?k->y:std::string(field)=="z"?k->z:std::string(field)=="r"?k->r:std::string(field)=="g"?k->g:k->b;return Value::real(v);});
}
void print_schema_summary(dslvm::Registry& reg){
  std::vector<std::string> order;std::map<std::string,std::vector<std::string>> members;std::string current;
  for(const auto& signature:reg.signatures()){
    if(signature.rfind("context ",0)==0)std::cout<<signature<<"\n";
    else if(signature.rfind("fn ",0)==0)std::cout<<signature<<"\n";
    else if(signature.rfind("method ",0)==0){std::string body=signature.substr(7),type=body.substr(0,body.find('.'));if(type!=current){current=type;order.push_back(type);}members[type].push_back(body);}
  }
  for(const auto& type:order){std::cout<<type<<":\n";for(const auto& member:members[type])std::cout<<"  "<<member<<"\n";}
}
}
int main(int argc,char** argv){
  Host host;
  dslvm::Registry reg;
  register_host(reg,host);
  const std::string usage="usage: lmcad --schema [--json] | --dsl-signatures | --dsl FILE [FILE ...]\n";
  if(argc>=2&&std::string(argv[1])=="--schema"){
    if(argc==2){print_schema_summary(reg);return std::cout?0:4;}
    if(argc==3&&std::string(argv[2])=="--json"){
      std::cout.write(reinterpret_cast<const char*>(lmcad::embedded_schema),static_cast<std::streamsize>(lmcad::embedded_schema_size));
      return std::cout?0:4;
    }
    std::cerr<<usage;return 2;
  }
  if(argc==2&&std::string(argv[1])=="--dsl-signatures") {
    for(const auto& signature:reg.signatures()) std::cout<<signature<<"\n";
    return 0;
  }
  if(argc<3||std::string(argv[1])!="--dsl") {
    std::cerr<<usage;
    return 2;
  }
  // Validate the complete invocation before any block can write output.
  for(int i=2;i<argc;i++) {
    if(std::string(argv[i]).empty()||std::string(argv[i]).rfind("--",0)==0) {
      std::cerr<<usage;
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
