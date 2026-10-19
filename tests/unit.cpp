#include "render.hpp"
#include "cad.hpp"
#include "atomic-file.hpp"
#include "deflate.hpp"
#include "inflate.hpp"
#include <filesystem>
#include <csignal>
#ifndef _WIN32
#include <sys/resource.h>
#endif
#include "keyframes.hpp"
#include <cstdint>
#include <cmath>
#include <limits>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <sstream>
#include <vector>
namespace {
uint32_t crc(const unsigned char* p,size_t n){uint32_t c=0xffffffffu;for(size_t i=0;i<n;i++){c^=p[i];for(int b=0;b<8;b++)c=(c>>1)^(0xedb88320u&uint32_t(-int(c&1)));}return c^0xffffffffu;}
uint32_t be32(const std::vector<unsigned char>& b,size_t i){return(uint32_t(b[i])<<24)|(uint32_t(b[i+1])<<16)|(uint32_t(b[i+2])<<8)|b[i+3];}
std::vector<unsigned char> read(const std::string& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("cannot read "+p);return {std::istreambuf_iterator<char>(f),{}};}
void check(bool ok,const char* what){if(!ok)throw std::runtime_error(what);}
struct ApngInfo { uint32_t frames=0, loops=0, width=0,height=0; uint16_t delay=0; std::vector<std::vector<unsigned char>> image_data; };
ApngInfo inspect_apng(const std::string& path){
 auto b=read(path);check(b.size()>8&&b[0]==137&&b[1]=='P'&&b[2]=='N'&&b[3]=='G',"bad APNG signature");
 size_t p=8;uint32_t next=0;bool ihdr=false,actl=false,iend=false,seen_idat=false;ApngInfo out;std::vector<unsigned char> current;
 while(p+12<=b.size()){
  uint32_t n=be32(b,p);p+=4;check(uint64_t(p)+4+n+4<=b.size(),"truncated APNG chunk");size_t start=p;std::string type(reinterpret_cast<const char*>(b.data()+p),4);p+=4;const auto* d=b.data()+p;
  check(crc(b.data()+start,4+n)==be32(b,p+n),"APNG chunk CRC mismatch");
  if(type=="IHDR"){check(!ihdr&&n==13,"bad APNG IHDR");out.width=be32(b,p);out.height=be32(b,p+4);ihdr=true;}
  else if(type=="sRGB")check(ihdr&&!actl&&n==1,"APNG sRGB ordering");
  else if(type=="acTL"){check(ihdr&&!actl&&!seen_idat&&n==8,"bad APNG acTL");actl=true;out.frames=be32(b,p);out.loops=be32(b,p+4);}
  else if(type=="fcTL"){
   check(actl&&n==26&&be32(b,p)==next++,"bad APNG fcTL sequence");
   if(!current.empty()){out.image_data.push_back(current);current.clear();}
   check(be32(b,p+4)>0&&be32(b,p+8)>0&&be32(b,p+12)==0&&be32(b,p+16)==0,"bad APNG frame rectangle");
   uint16_t delay=uint16_t(d[22])<<8|d[23];check(delay>0&&(uint16_t(d[20])<<8|d[21])==1,"bad APNG frame delay");check(d[24]==0&&d[25]==0,"bad APNG disposal/blend");if(!out.delay)out.delay=delay;check(out.delay==delay,"inconsistent APNG frame delays");
  }
  else if(type=="IDAT"){check(actl&&out.image_data.empty()&&next==1,"APNG IDAT order");seen_idat=true;current.insert(current.end(),d,d+n);}
  else if(type=="fdAT"){check(actl&&seen_idat&&n>4&&be32(b,p)==next++,"bad APNG fdAT sequence");current.insert(current.end(),d+4,d+n);}
  else if(type=="IEND"){check(n==0,"bad APNG IEND");if(!current.empty())out.image_data.push_back(current);iend=true;p+=n+4;break;}
  else check(type=="PLTE"||type=="tRNS","unexpected APNG chunk");
  p+=n+4;
 }
 check(ihdr&&actl&&iend&&p==b.size()&&out.frames==out.image_data.size(),"APNG chunk order/frame count mismatch");return out;
}
}
int main(int argc,char** argv){try{
 if(argc!=3)throw std::runtime_error("usage: lmcad-unit OUTPUT_DIR BUILD_DIR");
 std::string dir=argv[1],build=argv[2];
 { std::vector<unsigned char> repetitive(200000,42),noise(200000);uint32_t seed=12345;
   for(auto& v:noise){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;v=seed&255;}
   auto mixed=noise;mixed.insert(mixed.end(),noise.begin(),noise.begin()+32768);
   for(const auto& data:std::vector<std::vector<unsigned char>>{{},{0},{1,2},repetitive,noise,mixed}){
     auto encoded=lmcad::deflate_zlib(data);check(inflate_test(encoded)==data,"DEFLATE round trip");check(encoded==lmcad::deflate_zlib(data),"DEFLATE determinism");
   }
   check(lmcad::deflate_zlib(repetitive).size()<repetitive.size()/50,"DEFLATE compression ratio");
   check(lmcad::deflate_zlib(noise).size()<noise.size()+100,"DEFLATE stored fallback");
 }

 { const auto target=dir+"/atomic.png";
   {std::ofstream f(target);f<<"previous";}
   auto original=read(target);
   {lmcad::AtomicFile output(target);output.write({1,2,3});}
   check(read(target)==original,"abandoned write replaced destination");
#ifdef _WIN32
   // No RLIMIT_FSIZE/SIGXFSZ on Windows; the failed-write-destroys-nothing
   // guarantee is still exercised above (abandoned write) and in CI by the
   // host-level refusal checks.
   {lmcad::AtomicFile output(target);output.write(std::vector<unsigned char>(1024,42));output.commit();}
   check(read(target)!=original&&read(target).size()==1024,"committed write replaces destination");
#else
   struct rlimit oldLimit{};check(getrlimit(RLIMIT_FSIZE,&oldLimit)==0,"get file limit");
   auto limit=oldLimit;limit.rlim_cur=16;auto oldSignal=signal(SIGXFSZ,SIG_IGN);
   check(setrlimit(RLIMIT_FSIZE,&limit)==0,"set file limit");bool failed=false;
   try{lmcad::AtomicFile output(target);output.write(std::vector<unsigned char>(1024,42));output.commit();}catch(const std::exception&){failed=true;}
   setrlimit(RLIMIT_FSIZE,&oldLimit);signal(SIGXFSZ,oldSignal);
   check(failed&&read(target)==original,"failed write destroyed destination");
#endif
   for(const auto& entry:std::filesystem::directory_iterator(dir))check(entry.path().filename().string().rfind(".lmcad-",0)!=0,"failed output leaked temporary file");
 }
 for(auto named_mesh:std::vector<std::pair<std::string,lmcad::Mesh>>{{"box",lmcad::cad::tessellate(lmcad::cad::box(2,2,2))},{"sphere",lmcad::cad::tessellate(lmcad::cad::sphere(1))},{"cylinder",lmcad::cad::tessellate(lmcad::cad::cylinder(1,2))}}) {
   lmcad::Shape shape;shape.mesh=named_mesh.second;
   for(auto scale:{lmcad::Vec3{1,1,1},lmcad::Vec3{-1,1,1},lmcad::Vec3{-1,-1,1}}) {
     shape.scale=scale;
     for(auto t:lmcad::world_triangles({shape}))
       check(lmcad::dot(lmcad::cross(t.b-t.a,t.c-t.a),(t.a+t.b+t.c)/3)>-1e-8,(named_mesh.first+"/reflected winding points inward").c_str());
   }
 }
 // A reflected smooth triangle must match an explicitly reflected mesh, including its corner normals.
 lmcad::Shape reflected;reflected.scale={-1,1,1};
 lmcad::Triangle smooth{{-1,-1,0},{1,-1,0},{0,1,0}};
 smooth.na=lmcad::normalized({0,0,1});smooth.nb=lmcad::normalized({1,0,1});smooth.nc=lmcad::normalized({0,1,1});smooth.smooth=true;
 reflected.mesh.triangles={smooth};
 lmcad::Shape baked;auto rt=lmcad::transform_triangle(reflected,smooth);
 rt.na=lmcad::transform_normal(reflected,smooth.na);rt.nb=lmcad::transform_normal(reflected,smooth.nc);rt.nc=lmcad::transform_normal(reflected,smooth.nb);rt.smooth=true;baked.mesh.triangles={rt};
 lmcad::Camera reflectcam;reflectcam.eye={0,0,5};reflectcam.target={0,0,0};
 check(lmcad::render({reflected},reflectcam,{}, {},63,47).rgb==lmcad::render({baked},reflectcam,{}, {},63,47).rgb,"reflection swapped vertices without their smooth normals");
 for(double deflection:{0.0,1e-100,1000000.0,std::numeric_limits<double>::infinity()}){
   bool refused=false;try{lmcad::cad::tessellate(lmcad::cad::box(1,1,1),deflection);}catch(const std::exception&){refused=true;}
   check(refused,"unbounded tessellation deflection accepted");
 }
 for(double aspect:{0.1,1.0,3.0})for(double fov:{2.0,45.0,178.0}){
   auto fit=lmcad::fitted_camera({0,0,0},std::sqrt(3.0),fov,1.1,aspect);
   auto forward=lmcad::normalized(fit.target-fit.eye),right=lmcad::normalized(lmcad::cross(forward,{0,1,0})),up=lmcad::cross(right,forward);
   for(double x:{-1.0,1.0})for(double y:{-1.0,1.0})for(double z:{-1.0,1.0}){
     auto delta=lmcad::Vec3{x,y,z}-fit.eye;double depth=lmcad::dot(delta,forward),half=depth*std::tan(fov*std::acos(-1.0)/360);
     check(depth>fit.near_clip&&depth<fit.far_clip&&std::abs(lmcad::dot(delta,right))<half*aspect&&std::abs(lmcad::dot(delta,up))<half,"fitted camera clips bounds");
   }
 }
 auto tiny_fit=lmcad::fitted_camera({},1e-4,45,1.1);
 check(lmcad::length(tiny_fit.eye)<0.001&&tiny_fit.near_clip<1e-4,"tiny fitted geometry kept a fixed distance floor");
 bool fit_overflow=false;try{lmcad::fitted_camera({},100,45,std::numeric_limits<double>::max());}catch(const std::exception&){fit_overflow=true;}
 check(fit_overflow,"camera fit overflow accepted");
 lmcad::Camera resolve_camera;resolve_camera.eye={0,0,0};resolve_camera.target={0,0,-1};resolve_camera.fov_degrees=90;
 lmcad::Light unlit;unlit.ambient=1;
 lmcad::Shape half;half.material.color={1,1,1};half.mesh.triangles={{{-1,-1,-1},{0,-1,-1},{0,1,-1}},{{-1,-1,-1},{0,1,-1},{-1,1,-1}}};
 auto half_pixel=lmcad::render({half},resolve_camera,unlit,{0,0,0},1,1,2);
 check(half_pixel.rgb==std::vector<unsigned char>({188,188,188}),"resolve averages encoded bytes instead of linear light");
 for(int samples:{1,2,3,4}){
   auto native=lmcad::render({half},resolve_camera,unlit,{0.1,0.2,0.3},7*samples,5*samples,1);
   auto resolved=lmcad::render({half},resolve_camera,unlit,{0.1,0.2,0.3},7,5,samples);
   check(resolved.width==7&&resolved.height==5&&resolved.rgb.size()==105,"odd-size resolve dimensions");
   for(int y=0;y<5;++y)for(int x=0;x<7;++x)for(int c=0;c<3;++c){double sum=0;
     for(int dy=0;dy<samples;++dy)for(int dx=0;dx<samples;++dx){double v=native.rgb[((y*samples+dy)*7*samples+x*samples+dx)*3+c]/255.0;sum+=v<=0.04045?v/12.92:std::pow((v+0.055)/1.055,2.4);}
     double v=sum/(samples*samples);int expected=int(std::lround(255*(v<=0.0031308?12.92*v:1.055*std::pow(v,1/2.4)-0.055)));
     check(resolved.rgb[(y*7+x)*3+c]==expected,"resolve misses an edge sample or colour channel");
   }
 }
 for(auto dims:std::vector<std::array<int,3>>{{1,1,0},{1,1,5},{2049,1,4},{1025,1025,4},{2147483647,1,1}}){
   bool refused=false;try{lmcad::render({},resolve_camera,unlit,{},dims[0],dims[1],dims[2]);}catch(const std::exception&){refused=true;}
   check(refused,"supersampling allocation limits bypassed");
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
 lmcad::Shape precise;precise.mesh.triangles={{{10000000.25,0,0},{10000000.75,0,0},{10000000.25,1,0}}};
 lmcad::write_obj(dir+"/precise.obj",{precise});
 auto objbytes=read(dir+"/precise.obj");std::istringstream obj(std::string(objbytes.begin(),objbytes.end()));
 std::string line;std::getline(obj,line);std::getline(obj,line);std::istringstream vertex(line);char record;double vx;vertex>>record>>vx;
 check(record=='v'&&vx==10000000.25,"OBJ rounded away geometry detail");
 precise.name="bad\nf 999 999 999\r\n#";lmcad::write_obj(dir+"/name.obj",{precise});
 auto named=read(dir+"/name.obj");std::string namedtext(named.begin(),named.end());
 check(namedtext.find("\nf 999")==std::string::npos,"OBJ object name injected a face record");
 for(int format=0;format<4;format++) {
   std::string path=dir+"/nul";path.push_back('\0');path+="ignored";
   bool failed=false;try {
     if(format==0)lmcad::write_png(path,{1,1,{1,2,3}});
     if(format==1)lmcad::write_ppm(path,{1,1,{1,2,3}});
     if(format==2)lmcad::write_stl(path,{});
     if(format==3)lmcad::write_obj(path,{});
   }catch(const std::exception&){failed=true;}
   check(failed,"output path silently truncated at NUL");
 }
 lmcad::Shape huge;huge.mesh=lmcad::cad::tessellate(lmcad::cad::box(2,2,2));huge.scale={1e200,1e200,1e200};
 bool rejected=false;try{lmcad::write_stl(dir+"/huge.stl",{huge});}catch(const std::exception&){rejected=true;}
 check(rejected,"STL accepted coordinates outside float range");
 huge.scale={1e308,1e308,1e308};huge.translation={1e308,0,0};rejected=false;
 try{lmcad::transform_point(huge,{1,0,0});}catch(const std::exception&){rejected=true;}
 check(rejected,"transform overflow was accepted");
 const std::string header="t,x,y,z,r,g,b\n", row="0,0,0,0,1,0,0\n", last="1,1,0,0,0,0,1\n";
 auto csv=[&](const std::string& text){std::ofstream file(dir+"/keys.csv",std::ios::binary);file<<text;file.close();return lmcad::read_keyframes_csv(dir+"/keys.csv");};
 check(csv("\"t\",x,y,z,r,g,b\r\n\"0\",0,0,0,1,0,0\r\n1,1,0,0,0,0,1\r\n").size()==2,"quoted CRLF CSV rejected");
 for(const auto& text:std::vector<std::string>{"",header,header+row,header+"0,0,0,0,1,0\n"+last,header+"0,0,0,0,1,0,0,\n"+last,header+"0junk,0,0,0,1,0,0\n"+last,header+"nan,0,0,0,1,0,0\n"+last,header+"inf,0,0,0,1,0,0\n"+last,header+"\"0,0,0,0,1,0,0\n",header+std::string(4097,'0'),header+row+row}){
   bool failed=false;try{csv(text);}catch(const dslvm::Error&){failed=true;}check(failed,"invalid CSV accepted");
 }
 std::string many=header;for(int i=0;i<4097;i++)many+=std::to_string(i)+",0,0,0,1,0,0\n";
 bool tooMany=false;try{csv(many);}catch(const dslvm::Error&){tooMany=true;}check(tooMany,"CSV row limit ignored");
 lmcad::Shape box;box.mesh=lmcad::cad::tessellate(lmcad::cad::box(1.4,1.0,.8));box.material.color={.8,.2,.1};box.translation={.1,0,0};
 lmcad::Camera camera;camera.eye={3,2,5};camera.target={0,0,0};lmcad::Light light;light.direction={-1,-1,-1};
 auto a=lmcad::render({box},camera,light,{.1,.2,.3},64,48),b=lmcad::render({box},camera,light,{.1,.2,.3},64,48);
 check(a.rgb==b.rgb,"raster output is not deterministic");lmcad::write_png(dir+"/a.png",a);lmcad::write_png(dir+"/b.png",b);
 auto aa=lmcad::render({box},camera,light,{.1,.2,.3},64,48,2),ab=lmcad::render({box},camera,light,{.1,.2,.3},64,48,2);
 check(aa.rgb==ab.rgb&&aa.rgb.size()==a.rgb.size()&&aa.rgb!=a.rgb,"2x supersampling is nondeterministic, kept the wrong dimensions, or changed no samples");
 auto bytes=read(dir+"/a.png"),other=read(dir+"/b.png");check(bytes==other,"PNG output is not deterministic");
 check(bytes.size()>8&&bytes[0]==137&&bytes[1]=='P'&&bytes[2]=='N'&&bytes[3]=='G',"bad PNG signature");
 size_t pos=8;std::vector<unsigned char> compressed;bool ihdr=false,iend=false,srgb=false;
 while(pos+12<=bytes.size()){uint32_t n=be32(bytes,pos);pos+=4;size_t start=pos;check(pos+4+n+4<=bytes.size(),"truncated PNG chunk");std::string type(reinterpret_cast<char*>(bytes.data()+pos),4);pos+=4;const unsigned char* data=bytes.data()+pos;
  check(crc(bytes.data()+start,4+n)==be32(bytes,pos+n),"PNG chunk CRC mismatch");
  if(type=="IHDR"){check(n==13&&be32(bytes,pos)==64&&be32(bytes,pos+4)==48,"bad PNG IHDR");ihdr=true;}
  if(type=="sRGB"){check(n==1&&data[0]==0,"bad sRGB chunk");srgb=true;}
  if(type=="IDAT")compressed.insert(compressed.end(),data,data+n);
  pos+=n+4;if(type=="IEND"){iend=true;break;}
 }
 check(ihdr&&iend&&srgb&&!compressed.empty(),"missing required PNG chunks");check(compressed.size()>=6,"truncated zlib stream");
 auto raw=inflate_test(compressed);
 check(raw.size()==48*(64*3+1),"PNG decoded size");
 for(int y=0;y<48;y++){check(raw[y*193]==0,"PNG filter");for(int x=0;x<192;x++)check(raw[y*193+1+x]==a.rgb[y*192+x],"PNG decoded pixels differ");}
 {
   const auto target=dir+"/stream.apng";{std::ofstream f(target);f<<"previous";}auto original=read(target);
   {lmcad::ApngWriter writer(target,64,48,30);writer.add_frame(a);check(read(target)==original,"animation published before finish");}
   check(read(target)==original,"unfinished animation replaced output");
   lmcad::ApngWriter writer(target,64,48,30);bool failed=false;
   try{writer.finish();}catch(const std::exception&){failed=true;}check(failed,"empty animation finish accepted");
   writer.add_frame(a);writer.add_frame(b);writer.finish();
   auto saved=read(target);failed=false;try{writer.finish();}catch(const std::exception&){failed=true;}check(failed,"double finish accepted");
   failed=false;try{writer.add_frame(a);}catch(const std::exception&){failed=true;}check(failed&&read(target)==saved,"add after finish accepted");
   lmcad::write_apng(dir+"/batch.apng",{a,b},30);check(saved==read(dir+"/batch.apng"),"streamed APNG differs from batch output");
   lmcad::Image pixel;pixel.width=1;pixel.height=1;pixel.rgb={1,2,3};lmcad::ApngWriter limited(dir+"/limit.apng",1,1,240);
   for(int i=0;i<240;i++)limited.add_frame(pixel);
   failed=false;try{limited.check_frame();}catch(const std::exception&){failed=true;}check(failed,"animation frame limit ignored");
 }
 for(int fps:{1,24,30,60,239,240}){lmcad::write_apng(dir+"/rate.apng",{a,b},fps,3);auto rate=inspect_apng(dir+"/rate.apng");check(rate.delay==fps&&rate.frames==2&&rate.loops==3,"APNG exact rational FPS");}
 auto tween=inspect_apng(build+"/tween.apng"),turn=inspect_apng(build+"/turntable.apng");
 for(const auto* movie:{&tween,&turn})for(const auto& frame:movie->image_data){auto pixels=inflate_test(frame);check(pixels.size()==movie->height*(movie->width*3+1),"APNG decoded frame size");}
 check(tween.frames==30&&tween.loops==0&&tween.image_data.size()==30,"tween APNG frame/loop count");
 check(turn.frames==36&&turn.loops==0&&turn.image_data.size()==36,"turntable APNG frame/loop count");
 check(tween.width==160&&tween.height==120&&turn.width==160&&turn.height==120,"APNG dimensions");
 check(tween.delay==30&&turn.delay==24,"APNG frame delay does not match requested FPS");
 auto first_png=read(build+"/tween-0.png");pos=8;std::vector<unsigned char> first_idat;
 while(pos+12<=first_png.size()){uint32_t n=be32(first_png,pos);std::string type(reinterpret_cast<const char*>(first_png.data()+pos+4),4);if(type=="IDAT")first_idat.insert(first_idat.end(),first_png.begin()+pos+8,first_png.begin()+pos+8+n);pos+=size_t(n)+12;if(type=="IEND")break;}
 check(first_idat==tween.image_data[0],"APNG first frame differs from matching PNG render");
 check(tween.image_data.front()!=tween.image_data.back()&&turn.image_data.front()!=turn.image_data.back(),"APNG frames are not changing");
 std::cout<<"unit checks passed: raster deterministic, PNG CRC/Adler valid; APNG CRC/sequence/chunks valid (30+36 frames), first frame matches PNG\n";return 0;
 }catch(const std::exception& e){std::cerr<<"unit failure: "<<e.what()<<"\n";return 1;}}
