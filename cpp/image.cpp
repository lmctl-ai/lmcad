#include "render.hpp"
#include "atomic-file.hpp"
#include "deflate.hpp"
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
void chunk(std::vector<unsigned char>& out,const char* type,const std::vector<unsigned char>& data){put32(out,uint32_t(data.size()));size_t begin=out.size();for(int i=0;i<4;i++)out.push_back(type[i]);out.insert(out.end(),data.begin(),data.end());put32(out,crc32(out.data()+begin,out.size()-begin));}
void save(const std::string& path,const std::vector<unsigned char>& b){validate_output_path(path);AtomicFile f(path);f.write(b);f.commit();}
void validate_image(const Image& im){
  if(im.width<=0||im.height<=0||im.width>8192||im.height>8192||uint64_t(im.width)*im.height>16777216||im.rgb.size()!=size_t(im.width)*im.height*3)throw std::runtime_error("invalid image buffer");
}
std::vector<unsigned char> raw_frame(const Image& im){
  validate_image(im);
  std::vector<unsigned char> raw;raw.reserve(size_t(im.height)*(size_t(im.width)*3+1));for(int y=0;y<im.height;y++){raw.push_back(0);auto b=im.rgb.begin()+size_t(y)*im.width*3;raw.insert(raw.end(),b,b+size_t(im.width)*3);}return raw;
}
std::vector<unsigned char> signature(){return {137,80,78,71,13,10,26,10};}
std::vector<unsigned char> ihdr(const Image& im){std::vector<unsigned char> d;put32(d,im.width);put32(d,im.height);d.insert(d.end(),{8,2,0,0,0});return d;}
}
void write_png(const std::string& path,const Image& im){
  auto raw=raw_frame(im),z=deflate_zlib(raw);auto out=signature();chunk(out,"IHDR",ihdr(im));chunk(out,"sRGB",{0});chunk(out,"IDAT",z);chunk(out,"IEND",{});save(path,out);
}
struct ApngWriter::Impl {
  std::unique_ptr<AtomicFile> file;
  int width,height,fps;
  uint32_t loops,count=0,seq=0;
  bool finished=false,failed=false;
  long actl_offset=0;
  Impl(int w,int h,int rate,uint32_t repeat):width(w),height(h),fps(rate),loops(repeat){}
};
ApngWriter::ApngWriter(const std::string& path,int width,int height,int fps,uint32_t loops):impl_(new Impl(width,height,fps,loops)) {
  if(width<1||height<1||width>8192||height>8192||uint64_t(width)*height>16777216)throw std::runtime_error("APNG dimensions exceed limits");
  if(fps<1||fps>240)throw std::runtime_error("APNG fps must be between 1 and 240");
  impl_->file=std::make_unique<AtomicFile>(path);
  auto header=signature();Image im;im.width=width;im.height=height;chunk(header,"IHDR",ihdr(im));chunk(header,"sRGB",{0});
  impl_->actl_offset=static_cast<long>(header.size());
  std::vector<unsigned char> control;put32(control,0);put32(control,loops);chunk(header,"acTL",control);impl_->file->write(header);
}
ApngWriter::~ApngWriter()=default;
void ApngWriter::check_frame() const {
  const auto& s=*impl_;
  if(s.finished)throw std::runtime_error("animation is already finished");
  if(s.failed)throw std::runtime_error("animation write previously failed");
  if(s.count>=240)throw std::runtime_error("animation frame limit exceeded (240)");
  if(uint64_t(s.count+1)*s.width*s.height>64000000)throw std::runtime_error("animation total frame pixel limit exceeded (64000000)");
}
void ApngWriter::add_frame(const Image& im){
  check_frame();auto& s=*impl_;
  if(im.width!=s.width||im.height!=s.height)throw std::runtime_error("APNG frames must have identical dimensions");
  auto raw=raw_frame(im),z=deflate_zlib(raw);
  std::vector<unsigned char> out,control;
  put32(control,s.seq);put32(control,s.width);put32(control,s.height);put32(control,0);put32(control,0);put16(control,1);put16(control,s.fps);control.push_back(0);control.push_back(0);chunk(out,"fcTL",control);
  if(s.count==0)chunk(out,"IDAT",z);
  else{std::vector<unsigned char> data;put32(data,s.seq+1);data.insert(data.end(),z.begin(),z.end());chunk(out,"fdAT",data);}
  try{s.file->write(out);}catch(...){s.failed=true;s.file.reset();throw;}
  s.seq+=s.count?2:1;++s.count;
}
void ApngWriter::finish(){
  auto& s=*impl_;
  if(s.finished)throw std::runtime_error("animation is already finished");
  if(s.failed)throw std::runtime_error("animation write previously failed");
  if(!s.count)throw std::runtime_error("animation needs at least one frame");
  try{
    std::vector<unsigned char> tail;chunk(tail,"IEND",{});s.file->write(tail);
    std::vector<unsigned char> control,out;put32(control,s.count);put32(control,s.loops);chunk(out,"acTL",control);
    s.file->seek(s.actl_offset);s.file->write(out);s.file->commit();s.file.reset();s.finished=true;
  }catch(...){s.failed=true;s.file.reset();throw;}
}
void write_apng(const std::string& path,const std::vector<Image>& frames,int fps,uint32_t loops){
  if(frames.empty()||frames.size()>240)throw std::runtime_error("APNG frame count must be between 1 and 240");
  ApngWriter writer(path,frames.front().width,frames.front().height,fps,loops);
  for(const auto& frame:frames)writer.add_frame(frame);
  writer.finish();
}
void write_ppm(const std::string& path,const Image& im){validate_image(im);std::string header="P6\n"+std::to_string(im.width)+" "+std::to_string(im.height)+"\n255\n";std::vector<unsigned char>b(header.begin(),header.end());b.insert(b.end(),im.rgb.begin(),im.rgb.end());save(path,b);}
}
