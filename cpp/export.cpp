#include "render.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <iomanip>
#include <locale>
#include <stdexcept>

namespace lmcad {
namespace {
std::string obj_name(std::string name){
  if(name.empty())return "shape";
  for(char& c:name)if(static_cast<unsigned char>(c)<=32||c=='#'||c=='\\')c='_';
  return name;
}
void write_bytes(std::ofstream& f,const void* p,size_t n,const std::string& path){f.write(static_cast<const char*>(p),std::streamsize(n));if(!f)throw std::runtime_error("failed writing output file '"+path+"'");}
void le32(std::ofstream& f,uint32_t x,const std::string& p){unsigned char b[4]={static_cast<unsigned char>(x),static_cast<unsigned char>(x>>8),static_cast<unsigned char>(x>>16),static_cast<unsigned char>(x>>24)};write_bytes(f,b,4,p);}
void le16(std::ofstream& f,uint16_t x,const std::string& p){unsigned char b[2]={static_cast<unsigned char>(x),static_cast<unsigned char>(x>>8)};write_bytes(f,b,2,p);}
void f32(std::ofstream& f,double value,const std::string& p){if(!std::isfinite(value)||std::abs(value)>std::numeric_limits<float>::max())throw std::runtime_error("STL coordinate exceeds finite float range");float x=static_cast<float>(value);uint32_t u;static_assert(sizeof(u)==sizeof(x));std::memcpy(&u,&x,4);le32(f,u,p);}
}
void write_stl(const std::string& path,const std::vector<Shape>& shapes){
  validate_output_path(path);std::ofstream f(path,std::ios::binary);if(!f)throw std::runtime_error("cannot open output file '"+path+"'");char header[80]={};const char* tag="lmcad binary STL";std::memcpy(header,tag,std::strlen(tag));write_bytes(f,header,80,path);
  uint64_t count=0;for(const auto& s:shapes)if(s.visible)count+=s.mesh.triangles.size();if(count>UINT32_MAX)throw std::runtime_error("STL triangle count exceeds format limit");le32(f,uint32_t(count),path);
  for(const auto& s:shapes)if(s.visible)for(const auto& t:s.mesh.triangles){auto world=transform_triangle(s,t);Vec3 a=world.a,b=world.b,c=world.c,n{};try{n=normalized(cross(b-a,c-a));}catch(...){} f32(f,n.x,path);f32(f,n.y,path);f32(f,n.z,path);for(Vec3 v:{a,b,c}){f32(f,v.x,path);f32(f,v.y,path);f32(f,v.z,path);}le16(f,0,path);}
  f.close();if(!f)throw std::runtime_error("failed writing output file '"+path+"'");
}
void write_obj(const std::string& path,const std::vector<Shape>& shapes){
  validate_output_path(path);std::ofstream f(path);if(!f)throw std::runtime_error("cannot open output file '"+path+"'");f.imbue(std::locale::classic());f<<std::setprecision(std::numeric_limits<double>::max_digits10);size_t index=1;
  for(const auto& s:shapes)if(s.visible){f<<"o "<<obj_name(s.name)<<"\n";for(const auto& t:s.mesh.triangles){auto world=transform_triangle(s,t);for(Vec3 v:{world.a,world.b,world.c}){f<<"v "<<v.x<<' '<<v.y<<' '<<v.z<<'\n';}}for(size_t i=0;i<s.mesh.triangles.size();i++){f<<"f "<<index<<' '<<index+1<<' '<<index+2<<'\n';index+=3;}}
  f.close();if(!f)throw std::runtime_error("failed writing output file '"+path+"'");
}
}
