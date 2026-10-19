#pragma once
#include "scene.hpp"
#include <TopoDS_Shape.hxx>
#include <gp_Pnt2d.hxx>
#include <vector>
#include <array>

namespace lmcad::cad {
struct Sketch { std::vector<gp_Pnt2d> points; bool closed=false; };
TopoDS_Shape box(double x,double y,double z);
TopoDS_Shape sphere(double radius);
TopoDS_Shape cylinder(double radius,double height);
TopoDS_Shape boolean_union(const TopoDS_Shape&,const TopoDS_Shape&);
TopoDS_Shape cut(const TopoDS_Shape&,const TopoDS_Shape&);
TopoDS_Shape intersect(const TopoDS_Shape&,const TopoDS_Shape&);
TopoDS_Shape fillet(const TopoDS_Shape&,double radius);
TopoDS_Shape chamfer(const TopoDS_Shape&,double distance);
TopoDS_Shape hollow(const TopoDS_Shape&,double thickness,bool top);
TopoDS_Shape mirror(const TopoDS_Shape&,char axis);
TopoDS_Shape transform(const TopoDS_Shape&,Vec3 translation,Vec3 rotation,Vec3 scale);
TopoDS_Shape extrude(const Sketch&,double height);
TopoDS_Shape revolve_y(const Sketch&);
double volume(const TopoDS_Shape&);
std::array<double,6> bounds(const TopoDS_Shape&);
Mesh tessellate(const TopoDS_Shape&,double tolerance=0.05);
}
