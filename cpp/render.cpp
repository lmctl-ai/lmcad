#include "render.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <cstdint>
#include <stdexcept>

namespace lmcad {
namespace {
struct ScreenVertex { long double x,y,z; Vec3 normal; };
template<class Plane> std::vector<ScreenVertex> clip(const std::vector<ScreenVertex>& input,Plane plane) {
  std::vector<ScreenVertex> output;
  if(input.empty())return output;
  auto a=input.back();auto da=plane(a);
  for(auto b:input){auto db=plane(b);
    if((da>=0)!=(db>=0)){
      auto t=da/(da-db);
      output.push_back({a.x*(1-t)+b.x*t,a.y*(1-t)+b.y*t,a.z*(1-t)+b.z*t,a.normal*(1-t)+b.normal*t});
    }
    if(db>=0)output.push_back(b);
    a=b;da=db;
  }
  return output;
}
unsigned char channel(double v) { return static_cast<unsigned char>(std::lround(std::clamp(v,0.0,1.0)*255.0)); }
}
static Image render_native(const std::vector<Shape>& shapes,const Camera& camera,const Light& light,Color bg,int w,int h) {
  if(w<1||h<1||w>8192||h>8192||uint64_t(w)*h>16777216)throw std::runtime_error("image dimensions exceed limits");
  Image im; im.width=w; im.height=h; im.rgb.resize(static_cast<size_t>(w)*h*3);
  for(size_t i=0;i<static_cast<size_t>(w)*h;i++){im.rgb[3*i]=channel(bg.r);im.rgb[3*i+1]=channel(bg.g);im.rgb[3*i+2]=channel(bg.b);}
  std::vector<double> depth(static_cast<size_t>(w)*h,std::numeric_limits<double>::infinity());
  Vec3 forward=normalized(camera.target-camera.eye), right=normalized(cross(forward,{0,1,0})), up=normalized(cross(right,forward));
  long double focal=(0.5L*h)/std::tan(static_cast<long double>(camera.fov_degrees)*std::acos(-1.0L)/360.0L); Vec3 tolight=normalized(light.direction*-1.0), filllight=normalized(light.fill_direction*-1.0);
  for(const auto& shape:shapes) if(shape.visible) for(const auto& local:shape.mesh.triangles) {
    auto triangle=transform_triangle(shape,local);
    Vec3 p[3]={triangle.a,triangle.b,triangle.c};
    std::vector<ScreenVertex> polygon;
    Vec3 face_normal;try{face_normal=normalized(cross(p[1]-p[0],p[2]-p[0]));}catch(...){continue;}
    Vec3 normals[3]={face_normal,face_normal,face_normal};
    if(local.smooth)try{normals[0]=transform_normal(shape,local.na);normals[1]=transform_normal(shape,local.nb);normals[2]=transform_normal(shape,local.nc);}catch(...){normals[0]=normals[1]=normals[2]=face_normal;}
    // transform_triangle reverses reflected winding; keep normals paired with vertices.
    if ((shape.scale.x<0) ^ (shape.scale.y<0) ^ (shape.scale.z<0)) std::swap(normals[1],normals[2]);
    for(int pi=0;pi<3;pi++){auto point=p[pi];
      long double x=static_cast<long double>(point.x)-camera.eye.x,y=static_cast<long double>(point.y)-camera.eye.y,z=static_cast<long double>(point.z)-camera.eye.z;
      polygon.push_back({x*right.x+y*right.y+z*right.z,x*up.x+y*up.y+z*up.z,x*forward.x+y*forward.y+z*forward.z,normals[pi]});
    }
    polygon=clip(polygon,[&](auto v){return v.z-camera.near_clip;});
    polygon=clip(polygon,[&](auto v){return camera.far_clip-v.z;});
    polygon=clip(polygon,[&](auto v){return v.z*(w*.5L/focal)+v.x;});
    polygon=clip(polygon,[&](auto v){return v.z*(w*.5L/focal)-v.x;});
    polygon=clip(polygon,[&](auto v){return v.z*(h*.5L/focal)+v.y;});
    polygon=clip(polygon,[&](auto v){return v.z*(h*.5L/focal)-v.y;});
    if(polygon.size()<3)continue;
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
      long double wa=a/v[0].z,wb=b/v[1].z,wc=c/v[2].z,sum=wa+wb+wc;Vec3 normal;
      try{normal=normalized((v[0].normal*static_cast<double>(wa)+v[1].normal*static_cast<double>(wb)+v[2].normal*static_cast<double>(wc))/static_cast<double>(sum));}catch(...){normal=face_normal;}
      double diffuse=std::max(0.0,dot(normal,tolight))*light.intensity+std::max(0.0,dot(normal,filllight))*light.fill_intensity;double shade=light.ambient+(1.0-light.ambient)*std::min(1.0,diffuse);
      im.rgb[3*at]=channel(shape.material.color.r*shade);im.rgb[3*at+1]=channel(shape.material.color.g*shade);im.rgb[3*at+2]=channel(shape.material.color.b*shade);
    }
    }
  }
  return im;
}
Image render(const std::vector<Shape>& shapes,const Camera& camera,const Light& light,Color bg,int w,int h,int samples) {
  if(samples<1||samples>4)throw std::runtime_error("supersample must be between 1 and 4");
  if(w<1||h<1||w>8192||h>8192||uint64_t(w)*h>16777216)throw std::runtime_error("image dimensions exceed limits");
  const uint64_t sw=uint64_t(w)*samples, sh=uint64_t(h)*samples;
  if(sw>8192||sh>8192||sw*sh>16777216)throw std::runtime_error("supersampled image exceeds raster limits");
  const Camera view=camera.fit_radius>0 ? fitted_camera(camera.target,camera.fit_radius,camera.fov_degrees,camera.fit_margin,double(w)/h) : camera;
  if(samples==1)return render_native(shapes,view,light,bg,w,h);
  Image hi=render_native(shapes,view,light,bg,static_cast<int>(sw),static_cast<int>(sh));
  Image out;out.width=w;out.height=h;out.rgb.resize(static_cast<size_t>(w)*h*3);
  const unsigned divisor=samples*samples;
  for(int y=0;y<h;y++)for(int x=0;x<w;x++)for(int c=0;c<3;c++){
    unsigned sum=0;for(int dy=0;dy<samples;dy++)for(int dx=0;dx<samples;dx++)sum+=hi.rgb[(static_cast<size_t>(y*samples+dy)*hi.width+(x*samples+dx))*3+c];
    out.rgb[(static_cast<size_t>(y)*w+x)*3+c]=static_cast<unsigned char>((sum+divisor/2)/divisor);
  }
  return out;
}
}
