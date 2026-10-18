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
  template<class T> Value hold(std::vector<std::unique_ptr<T>>& arena,const char* type,T value){arena.push_back(std::make_unique<T>(std::move(value)));return Value::object(type,arena.back().get());}
};
template<class T> T* self(void* p){return static_cast<T*>(p);}
void finite(double x,const char* name){if(!std::isfinite(x))throw dslvm::Error(std::string(name)+" must be finite");}
void positive(double x,const char* name){finite(x,name);if(x<=0)throw dslvm::Error(std::string(name)+" must be positive");}
void color_range(double x,const char* name){finite(x,name);if(x<0||x>1)throw dslvm::Error(std::string(name)+" must be between 0 and 1");}
lmcad::Vec3 vec(const Args& a,size_t i){return {a[i].f,a[i+1].f,a[i+2].f};}
void register_host(dslvm::Registry& r,Host& h){
  r.context("scene","Scene");
  r.function("material",0,[&h](Args&){return h.hold(h.materials,"Material",lmcad::Material{});});
  r.method("Scene","box",4,[&h](void*,Args& a){for(int i=0;i<3;i++)positive(a[i].f,"box dimension");return h.hold(h.shapes,"Shape",lmcad::Shape{lmcad::make_box(a[0].f,a[1].f,a[2].f),*static_cast<lmcad::Material*>(a[3].obj)});});
  r.method("Scene","sphere",2,[&h](void*,Args& a){positive(a[0].f,"sphere radius");return h.hold(h.shapes,"Shape",lmcad::Shape{lmcad::make_sphere(a[0].f),*static_cast<lmcad::Material*>(a[1].obj)});});
  r.method("Scene","cylinder",3,[&h](void*,Args& a){positive(a[0].f,"cylinder radius");positive(a[1].f,"cylinder height");return h.hold(h.shapes,"Shape",lmcad::Shape{lmcad::make_cylinder(a[0].f,a[1].f),*static_cast<lmcad::Material*>(a[2].obj)});});
  r.method("Scene","camera",7,[&h](void*,Args& a){for(auto& x:a)finite(x.f,"camera argument");if(a[0].f<=0||a[0].f>=179)throw dslvm::Error("camera field of view must be between 0 and 179 degrees");h.camera={a[0].f,vec(a,1),vec(a,4),.01,10000};try{lmcad::normalized(h.camera.target-h.camera.eye);lmcad::normalized(lmcad::cross(h.camera.target-h.camera.eye,{0,1,0}));}catch(...){throw dslvm::Error("camera eye and target must differ and view direction must not be vertical");}h.has_camera=true;return Value::object("Camera",&h.camera);});
  r.method("Scene","directional_light",4,[&h](void*,Args& a){for(auto& x:a)finite(x.f,"directional light argument");if(a[3].f<0)throw dslvm::Error("light intensity must not be negative");try{h.light={lmcad::normalized(vec(a,0)),a[3].f};}catch(...){throw dslvm::Error("light direction must be nonzero");}h.has_light=true;return Value::object("Light",&h.light);});
  auto values=[&h](){std::vector<lmcad::Shape> s;s.reserve(h.shapes.size());for(const auto& p:h.shapes)s.push_back(*p);return s;};
  auto render=[&h,values](const std::string& path,int w,int height,bool png){if(!h.has_camera)throw dslvm::Error("render requires a camera");if(path.empty())throw dslvm::Error("output path must not be empty");if(w<1||height<1||w>8192||height>8192||uint64_t(w)*height>16777216)throw dslvm::Error("image dimensions exceed limits (1..8192 per side, 16777216 pixels total)");try{auto scene=values();auto image=lmcad::render(scene,h.camera,h.light,h.background,w,height);if(png)lmcad::write_png(path,image);else lmcad::write_ppm(path,image);}catch(const dslvm::Error&){throw;}catch(const std::exception& e){throw dslvm::Error(e.what());}};
  r.method("Scene","render_png",3,[render](void*,Args& a){render(a[0].s,a[1].i,a[2].i,true);return Value::none();});
  r.method("Scene","render_ppm",3,[render](void*,Args& a){render(a[0].s,a[1].i,a[2].i,false);return Value::none();});
  r.method("Scene","export_stl",1,[&h,values](void*,Args& a){if(a[0].s.empty())throw dslvm::Error("output path must not be empty");try{auto scene=values();lmcad::write_stl(a[0].s,scene);}catch(const std::exception& e){throw dslvm::Error(e.what());}return Value::none();});
  r.method("Scene","export_obj",1,[&h,values](void*,Args& a){if(a[0].s.empty())throw dslvm::Error("output path must not be empty");try{auto scene=values();lmcad::write_obj(a[0].s,scene);}catch(const std::exception& e){throw dslvm::Error(e.what());}return Value::none();});
  r.method("Scene","background",3,[&h](void* p,Args& a){for(int i=0;i<3;i++)color_range(a[i].f,"background channel");h.background={a[0].f,a[1].f,a[2].f};return Value::object("Scene",p);});
  r.method("Shape","translate",3,[](void* p,Args& a){for(auto& x:a)finite(x.f,"translation");self<lmcad::Shape>(p)->translation=self<lmcad::Shape>(p)->translation+vec(a,0);return Value::object("Shape",p);});
  r.method("Shape","rotate",3,[](void* p,Args& a){for(auto& x:a)finite(x.f,"rotation");self<lmcad::Shape>(p)->rotation=self<lmcad::Shape>(p)->rotation+vec(a,0);return Value::object("Shape",p);});
  r.method("Shape","scale",3,[](void* p,Args& a){for(auto& x:a)positive(std::abs(x.f),"scale magnitude");auto* s=self<lmcad::Shape>(p);s->scale={s->scale.x*a[0].f,s->scale.y*a[1].f,s->scale.z*a[2].f};return Value::object("Shape",p);});
  r.method("Shape","visible",1,[](void* p,Args& a){self<lmcad::Shape>(p)->visible=a[0].b;return Value::object("Shape",p);});
  r.method("Shape","name",1,[](void* p,Args& a){self<lmcad::Shape>(p)->name=a[0].s;return Value::object("Shape",p);});
  r.method("Camera","look_at",6,[&h](void* p,Args& a){for(auto& x:a)finite(x.f,"camera look_at argument");auto* c=self<lmcad::Camera>(p);c->eye=vec(a,0);c->target=vec(a,3);try{lmcad::normalized(c->target-c->eye);lmcad::normalized(lmcad::cross(c->target-c->eye,{0,1,0}));}catch(...){throw dslvm::Error("camera eye and target must differ and view direction must not be vertical");}return Value::object("Camera",p);});
  r.method("Camera","clipping",2,[](void* p,Args& a){positive(a[0].f,"near clipping plane");positive(a[1].f,"far clipping plane");if(a[1].f<=a[0].f)throw dslvm::Error("far clipping plane must be greater than near clipping plane");auto* c=self<lmcad::Camera>(p);c->near_clip=a[0].f;c->far_clip=a[1].f;return Value::object("Camera",p);});
  r.method("Material","color",3,[](void* p,Args& a){for(int i=0;i<3;i++)color_range(a[i].f,"material color channel");self<lmcad::Material>(p)->color={a[0].f,a[1].f,a[2].f};return Value::object("Material",p);});
  r.method("Material","roughness",1,[](void* p,Args& a){color_range(a[0].f,"roughness");self<lmcad::Material>(p)->roughness=a[0].f;return Value::object("Material",p);});
  r.method("Light","intensity",1,[](void* p,Args& a){finite(a[0].f,"light intensity");if(a[0].f<0)throw dslvm::Error("light intensity must not be negative");self<lmcad::Light>(p)->intensity=a[0].f;return Value::object("Light",p);});
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
