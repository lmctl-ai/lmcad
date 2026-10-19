#include "../cpp/atomic-file.hpp"
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
uint32_t be32(const std::vector<unsigned char>& b,size_t p){return(uint32_t(b[p])<<24)|(uint32_t(b[p+1])<<16)|(uint32_t(b[p+2])<<8)|b[p+3];}
uint32_t crc32(const unsigned char* p,size_t n){uint32_t c=0xffffffffu;for(size_t i=0;i<n;i++){c^=p[i];for(int k=0;k<8;k++)c=(c>>1)^(0xedb88320u&uint32_t(-int(c&1)));}return c^0xffffffffu;}
void put32(std::vector<unsigned char>& b,uint32_t v){b.push_back(v>>24);b.push_back(v>>16);b.push_back(v>>8);b.push_back(v);}
void chunk(std::vector<unsigned char>& out,const char* type,const std::vector<unsigned char>& data){put32(out,uint32_t(data.size()));size_t p=out.size();for(int i=0;i<4;i++)out.push_back(type[i]);out.insert(out.end(),data.begin(),data.end());put32(out,crc32(out.data()+p,out.size()-p));}
std::vector<unsigned char> read(const std::string& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("cannot open input");return {std::istreambuf_iterator<char>(f),{}};}
void write(const std::string& p,const std::vector<unsigned char>& b){lmcad::AtomicFile f(p);f.write(b);f.commit();}
}
int main(int argc,char** argv){try{
 if(argc!=4)throw std::runtime_error("usage: lmcad-apng-extract INPUT.apng FRAME_INDEX OUTPUT.png");
 std::string index=argv[2];if(index.empty()||index.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("FRAME_INDEX must be a nonnegative integer");size_t wanted=std::stoull(index);auto src=read(argv[1]);const unsigned char sig[]={137,80,78,71,13,10,26,10};
 if(src.size()<8||!std::equal(sig,sig+8,src.begin()))throw std::runtime_error("bad PNG signature");
 std::vector<unsigned char> out(sig,sig+8),ihdr,srgb,compressed;size_t p=8,frame=static_cast<size_t>(-1);bool have_actl=false,have_ihdr=false,have_iend=false,have_data=false,have_idat=false;uint32_t next=0,frames=0;
 while(p+12<=src.size()){
  uint32_t n=be32(src,p);if(uint64_t(p)+12+n>src.size())throw std::runtime_error("truncated PNG chunk");size_t t=p+4,d=p+8;std::string type(reinterpret_cast<const char*>(&src[t]),4);
  if(crc32(src.data()+t,4+n)!=be32(src,d+n))throw std::runtime_error("PNG CRC mismatch");
  if(!have_ihdr&&type!="IHDR")throw std::runtime_error("IHDR must be first");
  if(type=="IHDR"){if(have_ihdr||n!=13)throw std::runtime_error("invalid IHDR");ihdr.assign(src.begin()+d,src.begin()+d+n);have_ihdr=true;}
  else if(type=="sRGB")srgb.assign(src.begin()+d,src.begin()+d+n);
  else if(type=="acTL"){if(n!=8||have_actl||next||have_idat||be32(src,d)==0)throw std::runtime_error("invalid acTL");have_actl=true;frames=be32(src,d);}
  else if(type=="fcTL"){
   if(!have_actl||n!=26||be32(src,d)!=next++||(frame!=size_t(-1)&&!have_data))throw std::runtime_error("invalid fcTL sequence/order");
   frame++;have_data=false;
   if(be32(src,d+4)!=be32(ihdr,0)||be32(src,d+8)!=be32(ihdr,4)||be32(src,d+12)||be32(src,d+16)||src[d+24]||src[d+25])throw std::runtime_error("only full-canvas source-blended frames with no disposal are supported");
  }
  else if(type=="IDAT"){
   if(!have_actl||frame!=0)throw std::runtime_error("invalid IDAT order");have_idat=true;have_data=true;
   if(frame==wanted)compressed.insert(compressed.end(),src.begin()+d,src.begin()+d+n);
  }
  else if(type=="fdAT"){
   if(!have_idat||frame==0||frame==size_t(-1)||n<5||be32(src,d)!=next++)throw std::runtime_error("invalid fdAT sequence/order");have_data=true;
   if(frame==wanted)compressed.insert(compressed.end(),src.begin()+d+4,src.begin()+d+n);
  }
  else if(type=="IEND"){if(n||!have_data||frame+1!=frames||p+12!=src.size())throw std::runtime_error("invalid IEND/frame count");have_iend=true;break;}
  p+=12+n;
 }
 if(!have_ihdr||!have_actl||!have_iend||frame<wanted||compressed.empty())throw std::runtime_error("requested APNG frame not found");
 chunk(out,"IHDR",ihdr);if(!srgb.empty())chunk(out,"sRGB",srgb);chunk(out,"IDAT",compressed);chunk(out,"IEND",{});write(argv[3],out);return 0;
 }catch(const std::exception& e){std::cerr<<"lmcad-apng-extract: "<<e.what()<<"\n";return 2;}}
