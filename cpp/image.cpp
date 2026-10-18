#include "render.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace lmcad {
namespace {
void put32(std::vector<unsigned char>& o,uint32_t v){o.push_back(v>>24);o.push_back(v>>16);o.push_back(v>>8);o.push_back(v);}
uint32_t crc32(const unsigned char* p,size_t n){uint32_t c=0xffffffffu;for(size_t i=0;i<n;i++){c^=p[i];for(int b=0;b<8;b++)c=(c>>1)^(0xedb88320u&uint32_t(-int(c&1)));}return c^0xffffffffu;}
uint32_t adler32(const unsigned char* p,size_t n){uint32_t a=1,b=0;for(size_t i=0;i<n;i++){a=(a+p[i])%65521;b=(b+a)%65521;}return (b<<16)|a;}
void chunk(std::vector<unsigned char>& out,const char* type,const std::vector<unsigned char>& data){put32(out,uint32_t(data.size()));size_t begin=out.size();for(int i=0;i<4;i++)out.push_back(type[i]);out.insert(out.end(),data.begin(),data.end());put32(out,crc32(out.data()+begin,out.size()-begin));}
void save(const std::string& path,const std::vector<unsigned char>& b){std::ofstream f(path,std::ios::binary);if(!f)throw std::runtime_error("cannot open output file '"+path+"'");f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));f.close();if(!f)throw std::runtime_error("failed writing output file '"+path+"'");}
}
void write_png(const std::string& path,const Image& im){
  if(im.width<=0||im.height<=0||im.rgb.size()!=size_t(im.width)*im.height*3)throw std::runtime_error("invalid image buffer");
  std::vector<unsigned char> raw;raw.reserve(size_t(im.height)*(size_t(im.width)*3+1));for(int y=0;y<im.height;y++){raw.push_back(0);auto b=im.rgb.begin()+size_t(y)*im.width*3;raw.insert(raw.end(),b,b+size_t(im.width)*3);}
  std::vector<unsigned char> z={0x78,0x01};size_t pos=0;do{size_t n=std::min<size_t>(65535,raw.size()-pos);bool last=pos+n==raw.size();z.push_back(last?1:0);z.push_back(n&255);z.push_back((n>>8)&255);uint16_t inv=uint16_t(~uint16_t(n));z.push_back(inv&255);z.push_back(inv>>8);z.insert(z.end(),raw.begin()+pos,raw.begin()+pos+n);pos+=n;}while(pos<raw.size());put32(z,adler32(raw.data(),raw.size()));
  std::vector<unsigned char> out={137,80,78,71,13,10,26,10};std::vector<unsigned char> ihdr;put32(ihdr,im.width);put32(ihdr,im.height);ihdr.insert(ihdr.end(),{8,2,0,0,0});chunk(out,"IHDR",ihdr);chunk(out,"IDAT",z);chunk(out,"IEND",{});save(path,out);
}
void write_ppm(const std::string& path,const Image& im){std::string header="P6\n"+std::to_string(im.width)+" "+std::to_string(im.height)+"\n255\n";std::vector<unsigned char>b(header.begin(),header.end());b.insert(b.end(),im.rgb.begin(),im.rgb.end());save(path,b);}
}
