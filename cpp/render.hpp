#pragma once
#include "scene.hpp"
#include <cstdint>
#include <string>
#include <memory>
namespace lmcad {
void validate_output_path(const std::string& path);
Image render(const std::vector<Shape>& shapes, const Camera& camera, const Light& light, Color background, int width, int height, int supersample=1);
void write_png(const std::string& path, const Image& image);
class ApngWriter {
  struct Impl;
  std::unique_ptr<Impl> impl_;
public:
  ApngWriter(const std::string& path,int width,int height,int fps,uint32_t loop_count=0);
  ~ApngWriter();
  void check_frame() const; // Check cumulative limits before rendering/allocating a frame.
  void add_frame(const Image& image);
  void finish();
};
void write_apng(const std::string& path, const std::vector<Image>& frames, int fps, uint32_t loop_count=0);
void write_ppm(const std::string& path, const Image& image);
void write_stl(const std::string& path, const std::vector<Shape>& shapes);
void write_obj(const std::string& path, const std::vector<Shape>& shapes);
}
