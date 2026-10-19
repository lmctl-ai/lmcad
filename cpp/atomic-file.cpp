// Windows-side file helpers for AtomicFile. windows.h stays in this .cpp:
// its VOID typedef would collide with dslvm's Value::Kind enum in includers.
#include "atomic-file.hpp"
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <windows.h>
#include <random>

namespace lmcad::detail {
namespace {
std::wstring widen(const std::string& s) {
  if (s.empty()) return L"";
  int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
  std::wstring out(n, 0);
  MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), out.data(), n);
  return out;
}
// Names Win32 maps to something other than the file the path names: reserved
// devices (CON, NUL, AUX, COM1-LPT9), alternate data streams ("a.png:x"),
// trailing dots/spaces (silently stripped, aliasing the undecorated name).
bool windows_unsafe_component(const std::wstring& part) {
  if (part.empty() || part.back() == L'.' || part.back() == L' ') return true;
  for (wchar_t c : part)
    if (c < 0x20 || c == L':' || c == L'<' || c == L'>' || c == L'"' || c == L'|' ||
        c == L'?' || c == L'*')
      return true;
  std::wstring base = part.substr(0, part.find(L'.'));
  while (!base.empty() && base.back() == L' ') base.pop_back();
  for (auto& c : base) c = static_cast<wchar_t>(towupper(c));
  static const wchar_t* devices[] = {L"CON", L"PRN", L"AUX", L"NUL", L"CONIN$", L"CONOUT$"};
  for (const wchar_t* d : devices)
    if (base == d) return true;
  if (base.size() == 4 && (base.compare(0, 3, L"COM") == 0 || base.compare(0, 3, L"LPT") == 0) &&
      base[3] >= L'0' && base[3] <= L'9')
    return true;
  return false;
}
}

void check_windows_path(const std::string& path) {
  std::string forward = path;
  for (auto& c : forward) if (c == '\\') c = '/';
  size_t pos = 0;
  while (pos <= forward.size()) {
    size_t slash = forward.find('/', pos);
    std::string part = forward.substr(pos, slash == std::string::npos ? slash : slash - pos);
    // the drive root-name ("C:") is not a component; skip it and ../.
    if (!part.empty() && part != "." && part != ".." &&
        !(part.size() == 2 && std::isalpha(static_cast<unsigned char>(part[0])) && part[1] == ':') &&
        windows_unsafe_component(widen(part)))
      throw std::runtime_error("unsafe Windows path component '" + part + "' in: " + path);
    if (slash == std::string::npos) break;
    pos = slash + 1;
  }
}

int create_temp_file(const std::string& path, std::string& out_name) {
  auto slash = path.find_last_of("/\\");
  std::string dir = (slash == std::string::npos ? "." : path.substr(0, slash));
  static unsigned counter = 0;
  for (int attempt = 0; attempt < 100; attempt++) {
    std::string candidate = dir + "/.lmcad-" + std::to_string(_getpid()) + "-" +
                            std::to_string(++counter) + "-" +
                            std::to_string(std::random_device{}() & 0xffffff);
    HANDLE h = CreateFileW(widen(candidate).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
      if (GetLastError() == ERROR_FILE_EXISTS || GetLastError() == ERROR_ALREADY_EXISTS) continue;
      throw std::runtime_error("cannot create output: " + path);
    }
    int fd = _open_osfhandle((intptr_t)h, _O_WRONLY | _O_BINARY);
    if (fd < 0) {
      CloseHandle(h);
      DeleteFileW(widen(candidate).c_str());
      throw std::runtime_error("cannot open output: " + path);
    }
    out_name = candidate;
    return fd;
  }
  throw std::runtime_error("cannot create output: " + path);
}

void replace_file(const std::string& tmp, const std::string& dest) {
  // rename() cannot replace on Windows; MoveFileExW can, on one volume.
  if (!MoveFileExW(widen(tmp).c_str(), widen(dest).c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    throw std::runtime_error("MoveFileExW failed");
}

void sync_fd(int fd) { if (_commit(fd)) throw std::runtime_error("_commit failed"); }
void unlink_file(const std::string& path) { _unlink(path.c_str()); }
}
#endif
