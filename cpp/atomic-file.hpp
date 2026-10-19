#pragma once
#include <cerrno>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

namespace lmcad {
// Same-directory temporary output: failures never truncate an existing destination.
class AtomicFile {
  std::string destination_, temporary_;
  FILE* file_=nullptr;
public:
  explicit AtomicFile(const std::string& path):destination_(path) {
    if(path.empty()||path.find('\0')!=std::string::npos)throw std::runtime_error("invalid output path");
    struct stat st{};
    if(lstat(path.c_str(),&st)==0){if(!S_ISREG(st.st_mode))throw std::runtime_error("output must be a regular file: "+path);}
    else if(errno!=ENOENT)throw std::runtime_error("cannot inspect output: "+path);
    auto slash=path.find_last_of('/');
    std::string pattern=(slash==std::string::npos?"./":path.substr(0,slash+1))+".lmcad-XXXXXX";
    std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);
    int fd=mkstemp(name.data());if(fd<0)throw std::runtime_error("cannot create output: "+path);
    temporary_=name.data();file_=fdopen(fd,"w+b");
    if(!file_){close(fd);unlink(temporary_.c_str());throw std::runtime_error("cannot open output: "+path);}
  }
  AtomicFile(const AtomicFile&)=delete;
  AtomicFile& operator=(const AtomicFile&)=delete;
  ~AtomicFile(){if(file_)fclose(file_);if(!temporary_.empty())unlink(temporary_.c_str());}
  void write(const std::vector<unsigned char>& b){
    if(!file_||fwrite(b.data(),1,b.size(),file_)!=b.size())throw std::runtime_error("failed writing output: "+destination_);
  }
  void seek(long offset){if(!file_||fseek(file_,offset,SEEK_SET))throw std::runtime_error("failed seeking output: "+destination_);}
  void commit(){
    if(!file_)throw std::runtime_error("output already closed");
    bool failed=fflush(file_)!=0;
    if(!failed&&fsync(fileno(file_)))failed=true;
    if(fclose(file_))failed=true;
    file_=nullptr;
    if(failed)throw std::runtime_error("failed writing output: "+destination_);
    if(rename(temporary_.c_str(),destination_.c_str()))throw std::runtime_error("cannot replace output: "+destination_);
    temporary_.clear();
  }
};
}
