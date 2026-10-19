#pragma once
#include "dslvm.hpp"
#include <cmath>
#include <fstream>
#include <string>
#include <vector>

namespace lmcad {
struct Keyframe { double t,x,y,z,r,g,b; };
// Host-owned CSV parsing. The VM only receives schema object handles.
inline bool csv_record(std::istream& in,std::vector<std::string>& cells) {
  cells.clear();std::string cell;bool quoted=false,closed=false,seen=false;size_t bytes=0;
  while(true){
    int ch=in.get();
    if(ch==EOF){
      if(in.bad())throw dslvm::Error("cannot read keyframe CSV");
      if(quoted)throw dslvm::Error("unterminated quote in keyframe CSV");
      if(!seen)return false;
      cells.push_back(cell);return true;
    }
    seen=true;if(++bytes>4096)throw dslvm::Error("keyframe CSV record exceeds 4096 bytes");
    char c=static_cast<char>(ch);
    if(quoted){
      if(c=='"'){if(in.peek()=='"'){in.get();++bytes;cell+='"';}else{quoted=false;closed=true;}}
      else cell+=c;
    }else if(c==','||c=='\n'||c=='\r'){
      cells.push_back(cell);cell.clear();closed=false;
      if(c!=','){if(c=='\r'&&in.peek()=='\n')in.get();return true;}
    }else if(c=='"'&&cell.empty()&&!closed){quoted=true;}
    else {if(closed||c=='"')throw dslvm::Error("invalid quoting in keyframe CSV");cell+=c;}
  }
}
inline std::vector<Keyframe> read_keyframes_csv(const std::string& path){
  if(path.find('\0')!=std::string::npos)throw dslvm::Error("keyframe CSV path contains NUL");
  std::ifstream in(path,std::ios::binary);if(!in)throw dslvm::Error("cannot open keyframe CSV: "+path);
  std::vector<std::string> cells;
  if(!csv_record(in,cells)||cells!=std::vector<std::string>{"t","x","y","z","r","g","b"})throw dslvm::Error("keyframe CSV header must be t,x,y,z,r,g,b");
  std::vector<Keyframe> rows;
  while(csv_record(in,cells)){
    if(cells.size()==1&&cells[0].empty())continue;
    if(cells.size()!=7)throw dslvm::Error("keyframe CSV row must contain seven columns");
    double v[7];
    for(size_t i=0;i<7;i++){
      auto start=cells[i].find_first_not_of(" \t\r\n"),end=cells[i].find_last_not_of(" \t\r\n");
      if(start==std::string::npos)throw dslvm::Error("invalid number in keyframe CSV");
      auto cell=cells[i].substr(start,end-start+1);size_t used=0;
      try{v[i]=std::stod(cell,&used);}catch(const std::exception&){throw dslvm::Error("invalid number in keyframe CSV");}
      if(used!=cell.size()||!std::isfinite(v[i]))throw dslvm::Error("invalid finite number in keyframe CSV");
    }
    if(!rows.empty()&&v[0]<=rows.back().t)throw dslvm::Error("keyframe times must be strictly increasing");
    if(v[4]<0||v[4]>1||v[5]<0||v[5]>1||v[6]<0||v[6]>1)throw dslvm::Error("keyframe colors must be between 0 and 1");
    if(rows.size()>=4096)throw dslvm::Error("too many keyframes");
    rows.push_back({v[0],v[1],v[2],v[3],v[4],v[5],v[6]});
  }
  if(rows.size()<2)throw dslvm::Error("keyframe CSV needs at least two rows");
  return rows;
}
}
