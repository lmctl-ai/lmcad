#include "render.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace lmcad {
void validate_output_path(const std::string& path){
  if(path.empty()||path.find('\0')!=std::string::npos)throw std::runtime_error("output path must be nonempty and contain no NUL bytes");
}
namespace {
void put16(std::vector<unsigned char>& o,uint16_t v){o.push_back(v>>8);o.push_back(v);}
void put32(std::vector<unsigned char>& o,uint32_t v){o.push_back(v>>24);o.push_back(v>>16);o.push_back(v>>8);o.push_back(v);}
uint32_t crc32(const unsigned char* p,size_t n){uint32_t c=0xffffffffu;for(size_t i=0;i<n;i++){c^=p[i];for(int b=0;b<8;b++)c=(c>>1)^(0xedb88320u&uint32_t(-int(c&1)));}return c^0xffffffffu;}
uint32_t adler32(const unsigned char* p,size_t n){uint32_t a=1,b=0;for(size_t i=0;i<n;i++){a=(a+p[i])%65521;b=(b+a)%65521;}return (b<<16)|a;}
void chunk(std::vector<unsigned char>& out,const char* type,const std::vector<unsigned char>& data){put32(out,uint32_t(data.size()));size_t begin=out.size();for(int i=0;i<4;i++)out.push_back(type[i]);out.insert(out.end(),data.begin(),data.end());put32(out,crc32(out.data()+begin,out.size()-begin));}
void save(const std::string& path,const std::vector<unsigned char>& b){validate_output_path(path);std::ofstream f(path,std::ios::binary);if(!f)throw std::runtime_error("cannot open output file '"+path+"'");f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));f.close();if(!f)throw std::runtime_error("failed writing output file '"+path+"'");}
std::vector<unsigned char> raw_frame(const Image& im){
  if(im.width<=0||im.height<=0||im.width>8192||im.height>8192||uint64_t(im.width)*im.height>16777216||im.rgb.size()!=size_t(im.width)*im.height*3)throw std::runtime_error("invalid image buffer");
  std::vector<unsigned char> raw;raw.reserve(size_t(im.height)*(size_t(im.width)*3+1));for(int y=0;y<im.height;y++){raw.push_back(0);auto b=im.rgb.begin()+size_t(y)*im.width*3;raw.insert(raw.end(),b,b+size_t(im.width)*3);}return raw;
}
std::vector<unsigned char> stored_zlib(const std::vector<unsigned char>& raw){
  std::vector<unsigned char> z={0x78,0x01};size_t pos=0;do{size_t n=std::min<size_t>(65535,raw.size()-pos);bool last=pos+n==raw.size();z.push_back(last?1:0);z.push_back(n&255);z.push_back((n>>8)&255);uint16_t inv=uint16_t(~uint16_t(n));z.push_back(inv&255);z.push_back(inv>>8);z.insert(z.end(),raw.begin()+pos,raw.begin()+pos+n);pos+=n;}while(pos<raw.size());put32(z,adler32(raw.data(),raw.size()));return z;
}
std::vector<unsigned char> signature(){return {137,80,78,71,13,10,26,10};}
std::vector<unsigned char> ihdr(const Image& im){std::vector<unsigned char> d;put32(d,im.width);put32(d,im.height);d.insert(d.end(),{8,2,0,0,0});return d;}
}
void write_png(const std::string& path,const Image& im){
  auto raw=raw_frame(im),z=stored_zlib(raw);auto out=signature();chunk(out,"IHDR",ihdr(im));chunk(out,"sRGB",{0});chunk(out,"IDAT",z);chunk(out,"IEND",{});save(path,out);
}
void write_apng(const std::string& path,const std::vector<Image>& frames,int fps,uint32_t loop_count){
  if(frames.empty()||frames.size()>240)throw std::runtime_error("APNG frame count must be between 1 and 240");
  if(fps<1||fps>240)throw std::runtime_error("APNG fps must be between 1 and 240");
  const int w=frames.front().width,h=frames.front().height;
  if(uint64_t(w)*h*frames.size()>64000000)throw std::runtime_error("APNG total frame pixel limit exceeded (64000000)");
  std::vector<unsigned char> out=signature();chunk(out,"IHDR",ihdr(frames.front()));chunk(out,"sRGB",{0});
  std::vector<unsigned char> actl;put32(actl,static_cast<uint32_t>(frames.size()));put32(actl,loop_count);chunk(out,"acTL",actl);
  uint32_t seq=0;const uint16_t delay=static_cast<uint16_t>(std::max(1,int(std::lround(1000.0/fps))));
  for(size_t i=0;i<frames.size();i++){
    const auto& frame=frames[i];if(frame.width!=w||frame.height!=h)throw std::runtime_error("APNG frames must have identical dimensions");
    auto raw=raw_frame(frame),z=stored_zlib(raw);
    std::vector<unsigned char> fctl;put32(fctl,seq++);put32(fctl,w);put32(fctl,h);put32(fctl,0);put32(fctl,0);put16(fctl,delay);put16(fctl,1000);fctl.push_back(0);fctl.push_back(0);chunk(out,"fcTL",fctl);
    if(i==0)chunk(out,"IDAT",z);else{std::vector<unsigned char> fdat;put32(fdat,seq++);fdat.insert(fdat.end(),z.begin(),z.end());chunk(out,"fdAT",fdat);}
  }
  chunk(out,"IEND",{});save(path,out);
}
void write_ppm(const std::string& path,const Image& im){std::string header="P6\n"+std::to_string(im.width)+" "+std::to_string(im.height)+"\n255\n";std::vector<unsigned char>b(header.begin(),header.end());b.insert(b.end(),im.rgb.begin(),im.rgb.end());save(path,b);}
}
