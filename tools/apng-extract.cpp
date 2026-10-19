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
void write(const std::string& p,const std::vector<unsigned char>& b){std::ofstream f(p,std::ios::binary);if(!f)throw std::runtime_error("cannot open output");f.write(reinterpret_cast<const char*>(b.data()),static_cast<std::streamsize>(b.size()));if(!f)throw std::runtime_error("failed writing output");}
}
int main(int argc,char** argv){try{
 if(argc!=4)throw std::runtime_error("usage: lmcad-apng-extract INPUT.apng FRAME_INDEX OUTPUT.png");
 size_t wanted=std::stoull(argv[2]);auto src=read(argv[1]);const unsigned char sig[]={137,80,78,71,13,10,26,10};
 if(src.size()<8||!std::equal(sig,sig+8,src.begin()))throw std::runtime_error("bad PNG signature");
 std::vector<unsigned char> out(sig,sig+8),ihdr,srgb,compressed;size_t p=8,frame=static_cast<size_t>(-1);bool have_actl=false,have_ihdr=false,have_iend=false;
 while(p+12<=src.size()){
  uint32_t n=be32(src,p);if(uint64_t(p)+12+n>src.size())throw std::runtime_error("truncated PNG chunk");size_t t=p+4,d=p+8;std::string type(reinterpret_cast<const char*>(&src[t]),4);
  if(crc32(src.data()+t,4+n)!=be32(src,d+n))throw std::runtime_error("PNG CRC mismatch");
  if(type=="IHDR"){if(have_ihdr||n!=13)throw std::runtime_error("invalid IHDR");ihdr.assign(src.begin()+d,src.begin()+d+n);have_ihdr=true;}
  else if(type=="sRGB")srgb.assign(src.begin()+d,src.begin()+d+n);
  else if(type=="acTL"){if(n!=8)throw std::runtime_error("invalid acTL");have_actl=true;}
  else if(type=="fcTL"){if(n!=26)throw std::runtime_error("invalid fcTL");frame++;if(frame==wanted){uint32_t w=be32(src,d+4),h=be32(src,d+8);if(w!=be32(ihdr,0)||h!=be32(ihdr,4))throw std::runtime_error("subrect APNG frames are unsupported");}}
  else if(type=="IDAT"&&frame==wanted)compressed.insert(compressed.end(),src.begin()+d,src.begin()+d+n);
  else if(type=="fdAT"&&frame==wanted){if(n<4)throw std::runtime_error("invalid fdAT");compressed.insert(compressed.end(),src.begin()+d+4,src.begin()+d+n);}
  else if(type=="IEND"){have_iend=true;break;}
  p+=12+n;
 }
 if(!have_ihdr||!have_actl||!have_iend||frame<wanted||compressed.empty())throw std::runtime_error("requested APNG frame not found");
 chunk(out,"IHDR",ihdr);if(!srgb.empty())chunk(out,"sRGB",srgb);chunk(out,"IDAT",compressed);chunk(out,"IEND",{});write(argv[3],out);return 0;
 }catch(const std::exception& e){std::cerr<<"lmcad-apng-extract: "<<e.what()<<"\n";return 2;}}
