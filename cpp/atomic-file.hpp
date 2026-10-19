#pragma once
#include <cerrno>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif

namespace lmcad {
#ifdef _WIN32
// Windows API isolation: windows.h must stay out of this header (its VOID
// typedef collides with dslvm's Value::Kind enum for any later includer).
namespace detail {
// Refuse names Win32 would alias elsewhere (reserved devices, ADS, trailing
// dots/spaces). Throws std::runtime_error naming the component.
void check_windows_path(const std::string& path);
// Unguessable fresh same-directory temp file; returns fd (binary) + name.
int create_temp_file(const std::string& path, std::string& out_name);
void replace_file(const std::string& tmp, const std::string& dest); // MoveFileExW
void sync_fd(int fd); // _commit
void unlink_file(const std::string& path);
}
#endif
// Same-directory temporary output: failures never truncate an existing destination.
class AtomicFile {
  std::string destination_, temporary_;
  FILE* file_=nullptr;
public:
  explicit AtomicFile(const std::string& path):destination_(path) {
    if(path.empty()||path.find('\0')!=std::string::npos)throw std::runtime_error("invalid output path");
#ifdef _WIN32
    detail::check_windows_path(path);
    struct stat st{};
    if(stat(path.c_str(),&st)==0){if(!(st.st_mode&S_IFREG))throw std::runtime_error("output must be a regular file: "+path);}
    else if(errno!=ENOENT)throw std::runtime_error("cannot inspect output: "+path);
    int fd=detail::create_temp_file(path,temporary_);
    file_=fdopen(fd,"w+b");
    if(!file_){detail::unlink_file(temporary_);throw std::runtime_error("cannot open output: "+path);}
#else
    struct stat st{};
    if(lstat(path.c_str(),&st)==0){if(!S_ISREG(st.st_mode))throw std::runtime_error("output must be a regular file: "+path);}
    else if(errno!=ENOENT)throw std::runtime_error("cannot inspect output: "+path);
    auto slash=path.find_last_of('/');
    std::string pattern=(slash==std::string::npos?"./":path.substr(0,slash+1))+".lmcad-XXXXXX";
    std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);
    int fd=mkstemp(name.data());if(fd<0)throw std::runtime_error("cannot create output: "+path);
    temporary_=name.data();file_=fdopen(fd,"w+b");
    if(!file_){close(fd);unlink(temporary_.c_str());throw std::runtime_error("cannot open output: "+path);}
#endif
  }
  AtomicFile(const AtomicFile&)=delete;
  AtomicFile& operator=(const AtomicFile&)=delete;
  ~AtomicFile(){if(file_)fclose(file_);if(!temporary_.empty())
#ifdef _WIN32
    detail::unlink_file(temporary_);
#else
    unlink(temporary_.c_str());
#endif
  }
  void write(const std::vector<unsigned char>& b){
    if(!file_||fwrite(b.data(),1,b.size(),file_)!=b.size())throw std::runtime_error("failed writing output: "+destination_);
  }
  void seek(long offset){if(!file_||fseek(file_,offset,SEEK_SET))throw std::runtime_error("failed seeking output: "+destination_);}
  void commit(){
    if(!file_)throw std::runtime_error("output already closed");
    bool failed=fflush(file_)!=0;
#ifdef _WIN32
    if(!failed)try{detail::sync_fd(fileno(file_));}catch(...){failed=true;}
#else
    if(!failed&&fsync(fileno(file_)))failed=true;
#endif
    if(fclose(file_))failed=true;
    file_=nullptr;
    if(failed)throw std::runtime_error("failed writing output: "+destination_);
#ifdef _WIN32
    try{detail::replace_file(temporary_,destination_);}catch(const std::exception&){throw std::runtime_error("cannot replace output: "+destination_);}
#else
    if(rename(temporary_.c_str(),destination_.c_str()))throw std::runtime_error("cannot replace output: "+destination_);
#endif
    temporary_.clear();
  }
};
}
