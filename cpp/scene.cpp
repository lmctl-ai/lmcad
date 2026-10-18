#include "scene.hpp"
#include <cmath>
#include <utility>
#include <stdexcept>

namespace lmcad {
double dot(Vec3 a, Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
Vec3 cross(Vec3 a, Vec3 b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
double length(Vec3 a) { return std::hypot(a.x,a.y,a.z); }
Vec3 normalized(Vec3 a) { double n=length(a); if (!(n>0) || !std::isfinite(n)) throw std::runtime_error("cannot normalize zero or non-finite vector"); return a/n; }
static void add_quad(Mesh& m, Vec3 a, Vec3 b, Vec3 c, Vec3 d) { m.triangles.push_back({a,b,c}); m.triangles.push_back({a,c,d}); }
Mesh make_box(double x,double y,double z) {
  double a=x/2,b=y/2,c=z/2; Mesh m;
  add_quad(m,{-a,-b,c},{a,-b,c},{a,b,c},{-a,b,c}); add_quad(m,{a,-b,-c},{-a,-b,-c},{-a,b,-c},{a,b,-c});
  add_quad(m,{a,-b,c},{a,-b,-c},{a,b,-c},{a,b,c}); add_quad(m,{-a,-b,-c},{-a,-b,c},{-a,b,c},{-a,b,-c});
  add_quad(m,{-a,b,c},{a,b,c},{a,b,-c},{-a,b,-c}); add_quad(m,{-a,-b,-c},{a,-b,-c},{a,-b,c},{-a,-b,c});
  return m;
}
Mesh make_sphere(double r,int segments,int rings) {
  Mesh m; const double pi=std::acos(-1.0);
  auto point=[&](int i,int j){ double lat=-pi/2+pi*j/rings, lon=2*pi*i/segments; return Vec3{r*std::cos(lat)*std::cos(lon),r*std::sin(lat),r*std::cos(lat)*std::sin(lon)}; };
  for(int j=0;j<rings;j++) for(int i=0;i<segments;i++) { Vec3 a=point(i,j),b=point(i+1,j),c=point(i+1,j+1),d=point(i,j+1); if(j>0)m.triangles.push_back({a,d,b}); if(j+1<rings)m.triangles.push_back({b,d,c}); }
  return m;
}
Mesh make_cylinder(double r,double h,int n) {
  Mesh m; const double pi=std::acos(-1.0); double y=h/2;
  for(int i=0;i<n;i++){ double a=2*pi*i/n,b=2*pi*(i+1)/n; Vec3 p{r*std::cos(a),-y,r*std::sin(a)},q{r*std::cos(b),-y,r*std::sin(b)},u{q.x,y,q.z},v{p.x,y,p.z};
    m.triangles.push_back({p,u,q});m.triangles.push_back({p,v,u});m.triangles.push_back({{0,-y,0},p,q});m.triangles.push_back({{0,y,0},u,v}); }
  return m;
}
Vec3 transform_point(const Shape& s,Vec3 p) {
  p={p.x*s.scale.x,p.y*s.scale.y,p.z*s.scale.z};
  double cx=std::cos(s.rotation.x),sx=std::sin(s.rotation.x),cy=std::cos(s.rotation.y),sy=std::sin(s.rotation.y),cz=std::cos(s.rotation.z),sz=std::sin(s.rotation.z);
  p={p.x,p.y*cx-p.z*sx,p.y*sx+p.z*cx}; p={p.x*cy+p.z*sy,p.y,-p.x*sy+p.z*cy}; p={p.x*cz-p.y*sz,p.x*sz+p.y*cz,p.z};
  p=p+s.translation;
  if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))
    throw std::runtime_error("transformed coordinate must be finite");
  return p;
}
Triangle transform_triangle(const Shape& s,const Triangle& t) {
  Triangle out{transform_point(s,t.a),transform_point(s,t.b),transform_point(s,t.c)};
  if ((s.scale.x<0) ^ (s.scale.y<0) ^ (s.scale.z<0)) std::swap(out.b,out.c);
  return out;
}
std::vector<Triangle> world_triangles(const std::vector<Shape>& shapes) {
  std::vector<Triangle> out; size_t n=0; for(const auto& s:shapes) if(s.visible)n+=s.mesh.triangles.size(); out.reserve(n);
  for(const auto& s:shapes) if(s.visible) for(const auto& t:s.mesh.triangles) out.push_back(transform_triangle(s,t));
  return out;
}
}
