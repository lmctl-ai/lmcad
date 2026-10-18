#include "render.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <cstdint>
#include <stdexcept>

namespace lmcad {
namespace {
struct ScreenVertex { long double x,y,z; };
template<class Plane> std::vector<ScreenVertex> clip(const std::vector<ScreenVertex>& input,Plane plane) {
  std::vector<ScreenVertex> output;
  if(input.empty())return output;
  auto a=input.back();auto da=plane(a);
  for(auto b:input){auto db=plane(b);
    if((da>=0)!=(db>=0)){
      auto t=da/(da-db);
      output.push_back({a.x*(1-t)+b.x*t,a.y*(1-t)+b.y*t,a.z*(1-t)+b.z*t});
    }
    if(db>=0)output.push_back(b);
    a=b;da=db;
  }
  return output;
}
unsigned char channel(double v) { return static_cast<unsigned char>(std::lround(std::clamp(v,0.0,1.0)*255.0)); }
}
Image render(const std::vector<Shape>& shapes,const Camera& camera,const Light& light,Color bg,int w,int h) {
  if(w<1||h<1||w>8192||h>8192||uint64_t(w)*h>16777216)throw std::runtime_error("image dimensions exceed limits");
  Image im; im.width=w; im.height=h; im.rgb.resize(static_cast<size_t>(w)*h*3);
  for(size_t i=0;i<static_cast<size_t>(w)*h;i++){im.rgb[3*i]=channel(bg.r);im.rgb[3*i+1]=channel(bg.g);im.rgb[3*i+2]=channel(bg.b);}
  std::vector<double> depth(static_cast<size_t>(w)*h,std::numeric_limits<double>::infinity());
  Vec3 forward=normalized(camera.target-camera.eye), right=normalized(cross(forward,{0,1,0})), up=normalized(cross(right,forward));
  long double focal=(0.5L*h)/std::tan(static_cast<long double>(camera.fov_degrees)*std::acos(-1.0L)/360.0L); Vec3 tolight=normalized(light.direction*-1.0);
  for(const auto& shape:shapes) if(shape.visible) for(const auto& local:shape.mesh.triangles) {
    auto triangle=transform_triangle(shape,local);
    Vec3 p[3]={triangle.a,triangle.b,triangle.c};
    std::vector<ScreenVertex> polygon;
    for(auto point:p){
      long double x=static_cast<long double>(point.x)-camera.eye.x,y=static_cast<long double>(point.y)-camera.eye.y,z=static_cast<long double>(point.z)-camera.eye.z;
      polygon.push_back({x*right.x+y*right.y+z*right.z,x*up.x+y*up.y+z*up.z,x*forward.x+y*forward.y+z*forward.z});
    }
    polygon=clip(polygon,[&](auto v){return v.z-camera.near_clip;});
    polygon=clip(polygon,[&](auto v){return camera.far_clip-v.z;});
    polygon=clip(polygon,[&](auto v){return v.z*(w*.5L/focal)+v.x;});
    polygon=clip(polygon,[&](auto v){return v.z*(w*.5L/focal)-v.x;});
    polygon=clip(polygon,[&](auto v){return v.z*(h*.5L/focal)+v.y;});
    polygon=clip(polygon,[&](auto v){return v.z*(h*.5L/focal)-v.y;});
    if(polygon.size()<3)continue;
    Vec3 n; try{n=normalized(cross(normalized(p[1]-p[0]),normalized(p[2]-p[0])));}catch(...){continue;}
    double diffuse=std::max(0.0,dot(n,tolight))*light.intensity; double shade=.18+.82*std::min(1.0,diffuse);
    unsigned char color[3]={channel(shape.material.color.r*shade),channel(shape.material.color.g*shade),channel(shape.material.color.b*shade)};
    for(size_t fan=1;fan+1<polygon.size();fan++) {
      ScreenVertex v[3]={polygon[0],polygon[fan],polygon[fan+1]};
      for(auto& q:v){q.x=std::clamp(w*.5L+q.x*focal/q.z,0.0L,static_cast<long double>(w));q.y=std::clamp(h*.5L-q.y*focal/q.z,0.0L,static_cast<long double>(h));}
      long double area=(v[1].x-v[0].x)*(v[2].y-v[0].y)-(v[1].y-v[0].y)*(v[2].x-v[0].x);if(std::abs(area)<1e-12L)continue;
      int minx=int(std::floor(std::min({v[0].x,v[1].x,v[2].x}))),maxx=std::min(w-1,int(std::ceil(std::max({v[0].x,v[1].x,v[2].x}))));
      int miny=int(std::floor(std::min({v[0].y,v[1].y,v[2].y}))),maxy=std::min(h-1,int(std::ceil(std::max({v[0].y,v[1].y,v[2].y}))));
    for(int y=miny;y<=maxy;y++) for(int x=minx;x<=maxx;x++) { double px=x+.5,py=y+.5;
      long double a=((v[1].x-px)*(v[2].y-py)-(v[1].y-py)*(v[2].x-px))/area;
      long double b=((v[2].x-px)*(v[0].y-py)-(v[2].y-py)*(v[0].x-px))/area; long double c=1-a-b;
      if(a < -1e-9||b < -1e-9||c < -1e-9)continue;
      double zz=1.0/(a/v[0].z+b/v[1].z+c/v[2].z); size_t at=static_cast<size_t>(y)*w+x; if(zz>=depth[at])continue; depth[at]=zz;
      im.rgb[3*at]=color[0];im.rgb[3*at+1]=color[1];im.rgb[3*at+2]=color[2];
    }
    }
  }
  return im;
}
}
