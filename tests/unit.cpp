#include "render.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
namespace {
uint32_t crc(const unsigned char* p,size_t n){uint32_t c=0xffffffffu;for(size_t i=0;i<n;i++){c^=p[i];for(int b=0;b<8;b++)c=(c>>1)^(0xedb88320u&uint32_t(-int(c&1)));}return c^0xffffffffu;}
uint32_t adler(const std::vector<unsigned char>& v){uint32_t a=1,b=0;for(auto x:v){a=(a+x)%65521;b=(b+a)%65521;}return(b<<16)|a;}
uint32_t be32(const std::vector<unsigned char>& b,size_t i){return(uint32_t(b[i])<<24)|(uint32_t(b[i+1])<<16)|(uint32_t(b[i+2])<<8)|b[i+3];}
std::vector<unsigned char> read(const std::string& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("cannot read "+p);return {std::istreambuf_iterator<char>(f),{}};}
void check(bool ok,const char* what){if(!ok)throw std::runtime_error(what);}
}
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("usage: lmcad-unit OUTPUT_DIR");
 std::string dir=argv[1];
 for(auto mesh:{lmcad::make_box(2,2,2),lmcad::make_sphere(1),lmcad::make_cylinder(1,2)}) {
   lmcad::Shape shape;shape.mesh=mesh;
   for(auto scale:{lmcad::Vec3{1,1,1},lmcad::Vec3{-1,1,1},lmcad::Vec3{-1,-1,1}}) {
     shape.scale=scale;
     for(auto t:lmcad::world_triangles({shape}))
       check(lmcad::dot(lmcad::cross(t.b-t.a,t.c-t.a),(t.a+t.b+t.c)/3)>0,"primitive/reflected winding points inward");
   }
 }
 lmcad::Camera clipcam;clipcam.eye={0,0,0};clipcam.target={0,0,-1};clipcam.near_clip=1;clipcam.far_clip=4;
 lmcad::Shape crossing;crossing.mesh.triangles={{{-1,-1,-2},{1,-1,-2},{0,1,-.5}}};
 auto visible=[&](const lmcad::Shape& shape){auto im=lmcad::render({shape},clipcam,lmcad::Light{}, {0,0,0},32,32);size_t n=0;for(auto c:im.rgb)n+=c!=0;return n;};
 check(visible(crossing)>0,"near-plane crossing disappeared");
 crossing.mesh.triangles[0].c.z=-5;check(visible(crossing)>0,"far-plane crossing disappeared");
 crossing.mesh.triangles={{{-1e8,-1e8,-2},{1e8,-1e8,-2},{0,1e8,-2}}};
 check(visible(crossing)==32*32*3,"large offscreen triangle failed to cover viewport");
 lmcad::Shape front=crossing,back=crossing;front.material.color={1,0,0};back.material.color={0,1,0};back.translation={0,0,-1};
 auto frontFirst=lmcad::render({front,back},clipcam,{}, {0,0,0},32,32);
 auto backFirst=lmcad::render({back,front},clipcam,{}, {0,0,0},32,32);
 check(frontFirst.rgb==backFirst.rgb && frontFirst.rgb[0]>0 && frontFirst.rgb[1]==0,"depth depends on triangle order");
 if(std::ifstream("/dev/full").good()) {
   for(int format=0;format<4;format++) {
     bool failed=false;try {
       if(format==0)lmcad::write_png("/dev/full",{1,1,{1,2,3}});
       if(format==1)lmcad::write_ppm("/dev/full",{1,1,{1,2,3}});
       if(format==2)lmcad::write_stl("/dev/full",{});
       if(format==3)lmcad::write_obj("/dev/full",{front});
     }catch(const std::exception&){failed=true;}
     check(failed,"buffered output failure was not reported");
   }
 }
 lmcad::Shape huge;huge.mesh=lmcad::make_box(2,2,2);huge.scale={1e200,1e200,1e200};
 bool rejected=false;try{lmcad::write_stl(dir+"/huge.stl",{huge});}catch(const std::exception&){rejected=true;}
 check(rejected,"STL accepted coordinates outside float range");
 huge.scale={1e308,1e308,1e308};huge.translation={1e308,0,0};rejected=false;
 try{lmcad::transform_point(huge,{1,0,0});}catch(const std::exception&){rejected=true;}
 check(rejected,"transform overflow was accepted");
 lmcad::Shape box;box.mesh=lmcad::make_box(1.4,1.0,.8);box.material.color={.8,.2,.1};box.translation={.1,0,0};
 lmcad::Camera camera;camera.eye={3,2,5};camera.target={0,0,0};lmcad::Light light;light.direction={-1,-1,-1};
 auto a=lmcad::render({box},camera,light,{.1,.2,.3},64,48),b=lmcad::render({box},camera,light,{.1,.2,.3},64,48);
 check(a.rgb==b.rgb,"raster output is not deterministic");lmcad::write_png(dir+"/a.png",a);lmcad::write_png(dir+"/b.png",b);
 auto bytes=read(dir+"/a.png"),other=read(dir+"/b.png");check(bytes==other,"PNG output is not deterministic");
 check(bytes.size()>8&&bytes[0]==137&&bytes[1]=='P'&&bytes[2]=='N'&&bytes[3]=='G',"bad PNG signature");
 size_t pos=8;std::vector<unsigned char> compressed;bool ihdr=false,iend=false;
 while(pos+12<=bytes.size()){uint32_t n=be32(bytes,pos);pos+=4;size_t start=pos;check(pos+4+n+4<=bytes.size(),"truncated PNG chunk");std::string type(reinterpret_cast<char*>(bytes.data()+pos),4);pos+=4;const unsigned char* data=bytes.data()+pos;
  check(crc(bytes.data()+start,4+n)==be32(bytes,pos+n),"PNG chunk CRC mismatch");
  if(type=="IHDR"){check(n==13&&be32(bytes,pos)==64&&be32(bytes,pos+4)==48,"bad PNG IHDR");ihdr=true;}
  if(type=="IDAT")compressed.insert(compressed.end(),data,data+n);
  pos+=n+4;if(type=="IEND"){iend=true;break;}
 }
 check(ihdr&&iend&&!compressed.empty(),"missing required PNG chunks");check(compressed.size()>=6,"truncated zlib stream");
 std::vector<unsigned char> raw;size_t z=2;bool final=false;
 while(!final){check(z+5<=compressed.size()-4,"truncated stored deflate block");unsigned char hdr=compressed[z++];final=(hdr&1)!=0;check((hdr&6)==0,"PNG writer did not use stored deflate blocks");uint16_t len=uint16_t(compressed[z])|uint16_t(compressed[z+1])<<8;uint16_t inv=uint16_t(compressed[z+2])|uint16_t(compressed[z+3])<<8;z+=4;check(uint16_t(~len)==inv,"bad stored deflate length complement");check(z+len<=compressed.size()-4,"truncated stored deflate data");raw.insert(raw.end(),compressed.begin()+z,compressed.begin()+z+len);z+=len;}
 uint32_t expected=(uint32_t(compressed[compressed.size()-4])<<24)|(uint32_t(compressed[compressed.size()-3])<<16)|(uint32_t(compressed[compressed.size()-2])<<8)|compressed.back();check(adler(raw)==expected,"PNG Adler-32 mismatch");
 std::cout<<"unit checks passed: raster deterministic, PNG signature/IHDR/CRC-32/Adler-32 valid\n";return 0;
 }catch(const std::exception& e){std::cerr<<"unit failure: "<<e.what()<<"\n";return 1;}}
