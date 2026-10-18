#pragma once
#include "scene.hpp"
#include <string>
namespace lmcad {
Image render(const std::vector<Shape>& shapes, const Camera& camera, const Light& light, Color background, int width, int height);
void write_png(const std::string& path, const Image& image);
void write_ppm(const std::string& path, const Image& image);
void write_stl(const std::string& path, const std::vector<Shape>& shapes);
void write_obj(const std::string& path, const std::vector<Shape>& shapes);
}
