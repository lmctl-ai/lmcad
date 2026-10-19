#include <BRepAdaptor_Curve.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepGProp.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <GeomAbs_CurveType.hxx>
#include <Poly_Triangle.hxx>
#include <Poly_Triangulation.hxx>
#include <Standard_Version.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static void report(const std::string& name, const TopoDS_Shape& shape,
                   const std::string& stlPath) {
  // These are build123d Shape.mesh() defaults/semantics: relative deflection,
  // angular tolerance 0.5, and parallel meshing.
  BRepMesh_IncrementalMesh mesh(shape, 0.1, true, 0.5, true);
  if (!mesh.IsDone()) throw std::runtime_error("meshing failed: " + name);

  GProp_GProps volumeProps, areaProps;
  BRepGProp::VolumeProperties(shape, volumeProps);
  BRepGProp::SurfaceProperties(shape, areaProps);
  Bnd_Box bounds;
  BRepBndLib::AddOptimal(shape, bounds, false, false);
  double xmin, ymin, zmin, xmax, ymax, zmax;
  bounds.Get(xmin, ymin, zmin, xmax, ymax, zmax);

  struct Facet {
    std::array<float, 3> normal;
    std::array<std::array<float, 3>, 3> points;
    uint16_t attribute = 0;
  };
  std::vector<Facet> facets;
  for (TopExp_Explorer it(shape, TopAbs_FACE); it.More(); it.Next()) {
    const TopoDS_Face face = TopoDS::Face(it.Current());
    TopLoc_Location location;
    const Handle(Poly_Triangulation) triangulation =
        BRep_Tool::Triangulation(face, location);
    if (triangulation.IsNull()) continue;
    const gp_Trsf transform = location.Transformation();
    for (int i = 1; i <= triangulation->NbTriangles(); ++i) {
      int a, b, c;
      triangulation->Triangle(i).Get(a, b, c);
      if (face.Orientation() == TopAbs_REVERSED) std::swap(b, c);
      const gp_Pnt points[3] = {
          triangulation->Node(a).Transformed(transform),
          triangulation->Node(b).Transformed(transform),
          triangulation->Node(c).Transformed(transform)};
      gp_Vec normal(points[0], points[1]);
      normal.Cross(gp_Vec(points[0], points[2]));
      if (normal.SquareMagnitude() > 0.0) normal.Normalize();
      Facet facet;
      facet.normal = {float(normal.X()), float(normal.Y()), float(normal.Z())};
      for (int j = 0; j < 3; ++j)
        facet.points[j] = {float(points[j].X()), float(points[j].Y()),
                           float(points[j].Z())};
      facets.push_back(facet);
    }
  }

  // Tiny in-tree binary STL writer keeps TKDESTL's XCAF/data-exchange closure
  // out of this measurement. Binary STL is little-endian on this test host.
  std::ofstream out(stlPath, std::ios::binary);
  std::array<char, 80> header{};
  const std::string title = "OCCT " OCC_VERSION_STRING_EXT;
  std::memcpy(header.data(), title.data(), std::min(header.size(), title.size()));
  const uint32_t count = uint32_t(facets.size());
  out.write(header.data(), header.size());
  out.write(reinterpret_cast<const char*>(&count), sizeof(count));
  for (const Facet& facet : facets) {
    out.write(reinterpret_cast<const char*>(facet.normal.data()), 12);
    for (const auto& point : facet.points)
      out.write(reinterpret_cast<const char*>(point.data()), 12);
    out.write(reinterpret_cast<const char*>(&facet.attribute), 2);
  }
  if (!out) throw std::runtime_error("STL write failed: " + stlPath);
  out.close();

  std::cout << std::setprecision(12) << name << " volume=" << volumeProps.Mass()
            << " area=" << areaProps.Mass() << " bbox=" << xmin << ',' << ymin
            << ',' << zmin << ':' << xmax << ',' << ymax << ',' << zmax
            << " triangles=" << count << " stl=" << stlPath << '\n';
}

int main(int argc, char** argv) {
  const std::string out = argc > 1 ? argv[1] : ".";
  std::cout << "OCCT " << OCC_VERSION_STRING_EXT << '\n';

  const TopoDS_Shape box = BRepPrimAPI_MakeBox(40.0, 30.0, 20.0).Shape();
  BRepFilletAPI_MakeFillet fillet(box);
  for (TopExp_Explorer it(box, TopAbs_EDGE); it.More(); it.Next()) {
    const TopoDS_Edge edge = TopoDS::Edge(it.Current());
    BRepAdaptor_Curve curve(edge);
    if (curve.GetType() != GeomAbs_Line) continue;
    gp_Pnt point;
    gp_Vec tangent;
    curve.D1(curve.FirstParameter(), point, tangent);
    if (tangent.SquareMagnitude() > 0.0 &&
        std::abs(tangent.Z() / tangent.Magnitude()) > 0.999999)
      fillet.Add(3.0, edge);
  }
  fillet.Build();
  if (!fillet.IsDone()) throw std::runtime_error("vertical-edge fillet failed");

  const gp_Ax2 cutterAxis(gp_Pnt(20.0, 15.0, -1.0), gp_Dir(0.0, 0.0, 1.0));
  const TopoDS_Shape cutter = BRepPrimAPI_MakeCylinder(cutterAxis, 6.0, 22.0).Shape();
  BRepAlgoAPI_Cut cut(fillet.Shape(), cutter);
  cut.Build();
  if (!cut.IsDone()) throw std::runtime_error("cylindrical cut failed");

  BRepBuilderAPI_MakePolygon polygon;
  polygon.Add(gp_Pnt(0, 0, 0));
  polygon.Add(gp_Pnt(25, 0, 0));
  polygon.Add(gp_Pnt(20, 18, 0));
  polygon.Add(gp_Pnt(5, 22, 0));
  polygon.Close();
  const TopoDS_Shape extrusion = BRepPrimAPI_MakePrism(
      BRepBuilderAPI_MakeFace(polygon.Wire()).Face(), gp_Vec(0, 0, 10)).Shape();

  BRepBuilderAPI_MakePolygon profile;
  profile.Add(gp_Pnt(10, 0, 0));
  profile.Add(gp_Pnt(15, 0, 0));
  profile.Add(gp_Pnt(15, 0, 20));
  profile.Add(gp_Pnt(10, 0, 20));
  profile.Close();
  const TopoDS_Shape revolution = BRepPrimAPI_MakeRevol(
      BRepBuilderAPI_MakeFace(profile.Wire()).Face(),
      gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)), 2.0 * M_PI).Shape();

  report("box_fillet_hole", cut.Shape(), out + "/box_fillet_hole.stl");
  report("polygon_extrusion", extrusion, out + "/polygon_extrusion.stl");
  report("profile_revolution", revolution, out + "/profile_revolution.stl");
}
