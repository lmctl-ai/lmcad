#include "render.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace lmcad {
namespace {
struct ScreenVertex { double x,y,z; };
unsigned char channel(double v) { return static_cast<unsigned char>(std::lround(std::clamp(v,0.0,1.0)*255.0)); }
}
Image render(const std::vector<Shape>& shapes,const Camera& camera,const Light& light,Color bg,int w,int h) {
  Image im; im.width=w; im.height=h; im.rgb.resize(static_cast<size_t>(w)*h*3);
  for(size_t i=0;i<static_cast<size_t>(w)*h;i++){im.rgb[3*i]=channel(bg.r);im.rgb[3*i+1]=channel(bg.g);im.rgb[3*i+2]=channel(bg.b);}
  std::vector<double> depth(static_cast<size_t>(w)*h,std::numeric_limits<double>::infinity());
  Vec3 forward=normalized(camera.target-camera.eye), right=normalized(cross(forward,{0,1,0})), up=normalized(cross(right,forward));
  double focal=(0.5*h)/std::tan(camera.fov_degrees*std::acos(-1.0)/360.0); Vec3 tolight=normalized(light.direction*-1.0);
  for(const auto& shape:shapes) if(shape.visible) for(const auto& local:shape.mesh.triangles) {
    Vec3 p[3]={transform_point(shape,local.a),transform_point(shape,local.b),transform_point(shape,local.c)};
    Vec3 rel[3]={p[0]-camera.eye,p[1]-camera.eye,p[2]-camera.eye};
    double z[3]={dot(rel[0],forward),dot(rel[1],forward),dot(rel[2],forward)};
    if(z[0]<=camera.near_clip||z[1]<=camera.near_clip||z[2]<=camera.near_clip||z[0]>=camera.far_clip||z[1]>=camera.far_clip||z[2]>=camera.far_clip) continue;
    ScreenVertex v[3]; bool finite_projection=true; for(int i=0;i<3;i++){v[i]={w*.5+dot(rel[i],right)*focal/z[i],h*.5-dot(rel[i],up)*focal/z[i],z[i]};finite_projection=finite_projection&&std::isfinite(v[i].x)&&std::isfinite(v[i].y);} if(!finite_projection)continue;
    for(auto& q:v){q.x=std::clamp(q.x,-1000000.0,1000000.0);q.y=std::clamp(q.y,-1000000.0,1000000.0);}
    double area=(v[1].x-v[0].x)*(v[2].y-v[0].y)-(v[1].y-v[0].y)*(v[2].x-v[0].x); if(std::abs(area)<1e-12)continue;
    int minx=std::max(0,int(std::floor(std::min({v[0].x,v[1].x,v[2].x})))); int maxx=std::min(w-1,int(std::ceil(std::max({v[0].x,v[1].x,v[2].x}))));
    int miny=std::max(0,int(std::floor(std::min({v[0].y,v[1].y,v[2].y})))); int maxy=std::min(h-1,int(std::ceil(std::max({v[0].y,v[1].y,v[2].y}))));
    Vec3 n; try{n=normalized(cross(p[1]-p[0],p[2]-p[0]));}catch(...){continue;}
    double diffuse=std::max(0.0,dot(n,tolight))*light.intensity; double shade=.18+.82*std::min(1.0,diffuse);
    unsigned char color[3]={channel(shape.material.color.r*shade),channel(shape.material.color.g*shade),channel(shape.material.color.b*shade)};
    for(int y=miny;y<=maxy;y++) for(int x=minx;x<=maxx;x++) { double px=x+.5,py=y+.5;
      double a=((v[1].x-px)*(v[2].y-py)-(v[1].y-py)*(v[2].x-px))/area;
      double b=((v[2].x-px)*(v[0].y-py)-(v[2].y-py)*(v[0].x-px))/area; double c=1-a-b;
      if(a < -1e-9||b < -1e-9||c < -1e-9)continue;
      double zz=1.0/(a/z[0]+b/z[1]+c/z[2]); size_t at=static_cast<size_t>(y)*w+x; if(zz>=depth[at])continue; depth[at]=zz;
      im.rgb[3*at]=color[0];im.rgb[3*at+1]=color[1];im.rgb[3*at+2]=color[2];
    }
  }
  return im;
}
}
