#pragma once
#include "scene.hpp"
#include <cstdint>
#include <string>
namespace lmcad {
void validate_output_path(const std::string& path);
Image render(const std::vector<Shape>& shapes, const Camera& camera, const Light& light, Color background, int width, int height, int supersample=1);
void write_png(const std::string& path, const Image& image);
void write_apng(const std::string& path, const std::vector<Image>& frames, int fps, uint32_t loop_count=0);
void write_ppm(const std::string& path, const Image& image);
void write_stl(const std::string& path, const std::vector<Shape>& shapes);
void write_obj(const std::string& path, const std::vector<Shape>& shapes);
}
