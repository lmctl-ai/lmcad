#pragma once
#include <array>
#include <string>
#include <vector>

namespace lmcad {
struct Vec3 {
  double x=0, y=0, z=0;
  Vec3 operator+(Vec3 b) const { return {x+b.x,y+b.y,z+b.z}; }
  Vec3 operator-(Vec3 b) const { return {x-b.x,y-b.y,z-b.z}; }
  Vec3 operator*(double s) const { return {x*s,y*s,z*s}; }
  Vec3 operator/(double s) const { return {x/s,y/s,z/s}; }
};
double dot(Vec3 a, Vec3 b);
Vec3 cross(Vec3 a, Vec3 b);
double length(Vec3 a);
Vec3 normalized(Vec3 a);

struct Color { double r=.72, g=.74, b=.78; };
struct Triangle { Vec3 a,b,c; Vec3 na{},nb{},nc{}; bool smooth=false; };
struct Mesh { std::vector<Triangle> triangles; };
struct Material { Color color; double roughness=.7; };
struct Shape {
  Mesh mesh;
  Material material;
  Vec3 translation{};
  Vec3 rotation{};
  Vec3 scale{1,1,1};
  bool visible=true;
  std::string name;
};
struct Camera {
  double fov_degrees=45;
  Vec3 eye{3,3,5};
  Vec3 target{0,0,0};
  double near_clip=.01, far_clip=10000;
};
struct Light { Vec3 direction{-1,-1,-1}; double intensity=1; };
struct Image {
  int width=0, height=0;
  std::vector<unsigned char> rgb;
};

Triangle transform_triangle(const Shape& shape, const Triangle& triangle);
Vec3 transform_point(const Shape& shape, Vec3 point);
Vec3 transform_normal(const Shape& shape, Vec3 normal);
std::vector<Triangle> world_triangles(const std::vector<Shape>& shapes);
}
