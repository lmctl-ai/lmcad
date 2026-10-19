#pragma once
#include <cerrno>
#include <cstdio>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/stat.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace lmcad {
#ifdef _WIN32
// Names Win32 maps to something other than the file the path names: reserved
// devices (CON, NUL, COM1...), alternate data streams ("a.png:x"), and
// trailing dots/spaces (silently stripped, so "a.png." aliases "a.png").
inline bool windows_unsafe_component(const std::wstring& part) {
  if (part.empty() || part.back() == L'.' || part.back() == L' ') return true;
  for (wchar_t c : part)
    if (c < 0x20 || c == L':' || c == L'<' || c == L'>' || c == L'"' || c == L'|' ||
        c == L'?' || c == L'*')
      return true;
  std::wstring base = part.substr(0, part.find(L'.'));
  while (!base.empty() && base.back() == L' ') base.pop_back();
  for (auto& c : base) c = static_cast<wchar_t>(towupper(c));
  static const wchar_t* devices[] = {L"CON", L"PRN", L"NUL", L"CONIN$", L"CONOUT$"};
  for (const wchar_t* d : devices)
    if (base == d) return true;
  if (base == L"AUX") return true;
  if (base.size() == 4 && (base.compare(0, 3, L"COM") == 0 || base.compare(0, 3, L"LPT") == 0) &&
      base[3] >= L'0' && base[3] <= L'9')
    return true;
  return false;
}
inline std::wstring widen(const std::string& s) {
  if (s.empty()) return L"";
  int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
  std::wstring out(n, 0);
  MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), out.data(), n);
  return out;
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
    // Refuse names Win32 would alias to another file or device; the drive
    // root-name ("C:") is not a path component and never reaches here.
    std::string forward=path;for(auto&c:forward)if(c=='\\')c='/';
    size_t pos=0;while(pos<=forward.size()){
      size_t slash=forward.find('/',pos);
      std::string part=forward.substr(pos,slash==std::string::npos?slash:slash-pos);
      if(!part.empty()&&part!="."&&part!=".."&&windows_unsafe_component(widen(part)))
        throw std::runtime_error("unsafe Windows path component '"+part+"' in: "+path);
      if(slash==std::string::npos)break;pos=slash+1;
    }
    struct stat st{};
    if(stat(path.c_str(),&st)==0){if(!(st.st_mode&S_IFREG))throw std::runtime_error("output must be a regular file: "+path);}
    else if(errno!=ENOENT)throw std::runtime_error("cannot inspect output: "+path);
    auto slash=path.find_last_of("/\\");
    std::string dir=(slash==std::string::npos?".":path.substr(0,slash));
    // Unguessable same-directory temp via CreateFileW CREATE_NEW retries.
    static unsigned counter=0;
    int fd=-1;std::string candidate;
    for(int attempt=0;fd<0&&attempt<100;attempt++){
      candidate=dir+"/.lmcad-"+std::to_string(_getpid())+"-"+std::to_string(++counter)+"-"+
                std::to_string((unsigned)(std::random_device{}())&0xffffff);
      HANDLE h=CreateFileW(widen(candidate).c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH,nullptr);
      if(h==INVALID_HANDLE_VALUE){if(GetLastError()!=ERROR_FILE_EXISTS&&GetLastError()!=ERROR_ALREADY_EXISTS)
        throw std::runtime_error("cannot create output: "+path);continue;}
      fd=_open_osfhandle((intptr_t)h,_O_WRONLY|_O_BINARY);
      if(fd<0){CloseHandle(h);DeleteFileW(widen(candidate).c_str());throw std::runtime_error("cannot open output: "+path);}
    }
    if(fd<0)throw std::runtime_error("cannot create output: "+path);
    temporary_=candidate;file_=fdopen(fd,"w+b");
    if(!file_){_close(fd);_unlink(temporary_.c_str());throw std::runtime_error("cannot open output: "+path);}
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
    _unlink(temporary_.c_str());
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
    if(!failed&&_commit(fileno(file_)))failed=true;
#else
    if(!failed&&fsync(fileno(file_)))failed=true;
#endif
    if(fclose(file_))failed=true;
    file_=nullptr;
    if(failed)throw std::runtime_error("failed writing output: "+destination_);
#ifdef _WIN32
    // rename() cannot replace on Windows; MoveFileExW can, on one volume.
    if(!MoveFileExW(widen(temporary_).c_str(),widen(destination_).c_str(),
                    MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
      throw std::runtime_error("cannot replace output: "+destination_);
#else
    if(rename(temporary_.c_str(),destination_.c_str()))throw std::runtime_error("cannot replace output: "+destination_);
#endif
    temporary_.clear();
  }
};
}
