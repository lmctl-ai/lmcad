#include "scene.hpp"
#include <cmath>
#include <algorithm>
#include <utility>
#include <stdexcept>

namespace lmcad {
Camera fitted_camera(Vec3 center,double radius,double fov,double margin,double aspect){
  if(!std::isfinite(fov)||fov<=1||fov>=179||!std::isfinite(margin)||margin<1||
     !std::isfinite(radius)||radius<=0||!std::isfinite(aspect)||aspect<=0)
    throw std::runtime_error("invalid camera fit parameters");
  const double vertical=fov*std::acos(-1.0)/360.0;
  const double half=std::min(vertical,std::atan(std::tan(vertical)*aspect));
  const double distance=std::max(1.0,radius/std::sin(half)*margin);
  Camera c;c.fov_degrees=fov;c.target=center;c.eye=center+normalized({1.4,1.1,1.7})*distance;
  c.near_clip=std::min(.01,(distance-radius)*.5);c.far_clip=distance+radius*4;
  if(!std::isfinite(c.eye.x)||!std::isfinite(c.eye.y)||!std::isfinite(c.eye.z)||
     !std::isfinite(c.far_clip)||!(c.near_clip>0))
    throw std::runtime_error("camera fit exceeds finite coordinate range");
  c.fit_radius=radius;c.fit_margin=margin;return c;
}
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
