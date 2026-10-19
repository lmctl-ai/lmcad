#include "scene.hpp"
#include <cmath>
#include <utility>
#include <stdexcept>

namespace lmcad {
double dot(Vec3 a, Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
Vec3 cross(Vec3 a, Vec3 b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
double length(Vec3 a) { return std::hypot(a.x,a.y,a.z); }
Vec3 normalized(Vec3 a) { double n=length(a); if (!(n>0) || !std::isfinite(n)) throw std::runtime_error("cannot normalize zero or non-finite vector"); return a/n; }
Vec3 transform_point(const Shape& s,Vec3 p) {
  p={p.x*s.scale.x,p.y*s.scale.y,p.z*s.scale.z};
  double cx=std::cos(s.rotation.x),sx=std::sin(s.rotation.x),cy=std::cos(s.rotation.y),sy=std::sin(s.rotation.y),cz=std::cos(s.rotation.z),sz=std::sin(s.rotation.z);
  p={p.x,p.y*cx-p.z*sx,p.y*sx+p.z*cx}; p={p.x*cy+p.z*sy,p.y,-p.x*sy+p.z*cy}; p={p.x*cz-p.y*sz,p.x*sz+p.y*cz,p.z};
  p=p+s.translation;
  if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))
    throw std::runtime_error("transformed coordinate must be finite");
  return p;
}
Vec3 transform_normal(const Shape& s,Vec3 n) {
  n={n.x/s.scale.x,n.y/s.scale.y,n.z/s.scale.z};
  double cx=std::cos(s.rotation.x),sx=std::sin(s.rotation.x),cy=std::cos(s.rotation.y),sy=std::sin(s.rotation.y),cz=std::cos(s.rotation.z),sz=std::sin(s.rotation.z);
  n={n.x,n.y*cx-n.z*sx,n.y*sx+n.z*cx};n={n.x*cy+n.z*sy,n.y,-n.x*sy+n.z*cy};n={n.x*cz-n.y*sz,n.x*sz+n.y*cz,n.z};return normalized(n);
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
