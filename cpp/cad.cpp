#include "cad.hpp"
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <Bnd_Box.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepGProp.hxx>
#include <BRepTools.hxx>
#include <GProp_GProps.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <NCollection_List.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Trsf.hxx>
#include <gp_GTrsf.hxx>
#include <gp_Mat.hxx>
#include <Standard_Failure.hxx>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <map>
#include <tuple>

namespace lmcad::cad {
namespace {
// Host geometry envelope: keep OCCT's absolute-deflection meshing and its
// coordinate hashing within a practical, finite domain before allocation.
constexpr double max_extent=1000.0, max_coordinate=1000000.0;
void dimension(double v){if(!std::isfinite(v)||v<1e-5||v>max_extent)throw std::runtime_error("CAD dimension must be between 0.00001 and 1000");}
void envelope(const std::array<double,6>& b){
  for(double v:b)if(!std::isfinite(v)||std::abs(v)>max_coordinate)throw std::runtime_error("CAD coordinates exceed host limits");
  for(int i=0;i<3;++i)if(b[i+3]-b[i]>max_extent)throw std::runtime_error("CAD extent exceeds host limit (1000)");
}
void require_shape(const TopoDS_Shape& s,const char* operation){if(s.IsNull())throw std::runtime_error(std::string(operation)+" produced an empty shape");}
TopoDS_Shape checked(const TopoDS_Shape& s,const char* name){require_shape(s,name);return s;}
void require_sketch(const Sketch& s){
  if(!s.closed||s.points.size()<3)throw std::runtime_error("sketch must be closed and contain at least three points");
  auto orient=[](const gp_Pnt2d& a,const gp_Pnt2d& b,const gp_Pnt2d& c){return
    (static_cast<long double>(b.X())-a.X())*(static_cast<long double>(c.Y())-a.Y())-
    (static_cast<long double>(b.Y())-a.Y())*(static_cast<long double>(c.X())-a.X());};
  auto on=[](const gp_Pnt2d& a,const gp_Pnt2d& b,const gp_Pnt2d& p){return
    p.X()>=std::min(a.X(),b.X())&&p.X()<=std::max(a.X(),b.X())&&
    p.Y()>=std::min(a.Y(),b.Y())&&p.Y()<=std::max(a.Y(),b.Y());};
  long double area=0;
  const size_t n=s.points.size();
  for(const auto& p:s.points)if(!std::isfinite(p.X())||!std::isfinite(p.Y())||std::abs(p.X())>max_extent/2||std::abs(p.Y())>max_extent/2)
    throw std::runtime_error("CAD sketch coordinates exceed host limits (500)");
  for(size_t i=0;i<n;++i)if(s.points[i].Distance(s.points[(i+1)%n])<=1e-7)
    throw std::runtime_error("sketch contains a zero-length edge");
  for(size_t i=0;i<n;++i){
    const auto& a=s.points[i];const auto& b=s.points[(i+1)%n];
    area+=orient(s.points[0],a,b);
    for(size_t j=i+1;j<n;++j){
      if(j==i+1||(i==0&&j==n-1))continue;
      const auto& c=s.points[j];const auto& d=s.points[(j+1)%n];
      auto ab_c=orient(a,b,c),ab_d=orient(a,b,d),cd_a=orient(c,d,a),cd_b=orient(c,d,b);
      if(((ab_c>0&&ab_d<0)||(ab_c<0&&ab_d>0))&&((cd_a>0&&cd_b<0)||(cd_a<0&&cd_b>0)))
        throw std::runtime_error("sketch is self-intersecting");
      if((ab_c==0&&on(a,b,c))||(ab_d==0&&on(a,b,d))||(cd_a==0&&on(c,d,a))||(cd_b==0&&on(c,d,b)))
        throw std::runtime_error("sketch is self-intersecting");
    }
  }
  if(std::abs(area)<=1e-14L)throw std::runtime_error("sketch has zero area");
}
}
TopoDS_Shape box(double x,double y,double z){dimension(x);dimension(y);dimension(z);gp_Trsf t;t.SetTranslation(gp_Vec(-x/2,-y/2,-z/2));return BRepBuilderAPI_Transform(BRepPrimAPI_MakeBox(x,y,z).Shape(),t,true).Shape();}
TopoDS_Shape sphere(double r){dimension(2*r);return BRepPrimAPI_MakeSphere(r).Shape();}
TopoDS_Shape cylinder(double r,double h){dimension(2*r);dimension(h);return BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(0,-h/2,0),gp_Dir(0,1,0)),r,h).Shape();}
TopoDS_Shape boolean_union(const TopoDS_Shape& a,const TopoDS_Shape& b){BRepAlgoAPI_Fuse op(a,b);op.Build();if(!op.IsDone())throw std::runtime_error("union failed");return checked(op.Shape(),"union");}
TopoDS_Shape cut(const TopoDS_Shape& a,const TopoDS_Shape& b){BRepAlgoAPI_Cut op(a,b);op.Build();if(!op.IsDone())throw std::runtime_error("cut failed");return checked(op.Shape(),"cut");}
TopoDS_Shape intersect(const TopoDS_Shape& a,const TopoDS_Shape& b){BRepAlgoAPI_Common op(a,b);op.Build();if(!op.IsDone())throw std::runtime_error("intersection failed");return checked(op.Shape(),"intersection");}
TopoDS_Shape fillet(const TopoDS_Shape& s,double r){dimension(r);BRepFilletAPI_MakeFillet op(s);for(TopExp_Explorer e(s,TopAbs_EDGE);e.More();e.Next())op.Add(r,TopoDS::Edge(e.Current()));op.Build();if(!op.IsDone())throw std::runtime_error("fillet failed; radius may be too large");return checked(op.Shape(),"fillet");}
TopoDS_Shape chamfer(const TopoDS_Shape& s,double d){dimension(d);BRepFilletAPI_MakeChamfer op(s);for(TopExp_Explorer e(s,TopAbs_EDGE);e.More();e.Next())op.Add(d,TopoDS::Edge(e.Current()));op.Build();if(!op.IsDone())throw std::runtime_error("chamfer failed; distance may be too large");return checked(op.Shape(),"chamfer");}
TopoDS_Shape hollow(const TopoDS_Shape& s,double thickness,bool top){
  dimension(thickness);
  TopoDS_Face selected; double best=top?-INFINITY:INFINITY;
  for(TopExp_Explorer e(s,TopAbs_FACE);e.More();e.Next()) {auto f=TopoDS::Face(e.Current());BRepAdaptor_Surface surface(f,true);if(surface.GetType()!=GeomAbs_Plane)continue;GProp_GProps p;BRepGProp::SurfaceProperties(f,p);const gp_Pnt c=p.CentreOfMass();double v=c.Y();if(top?v>best:v<best){best=v;selected=f;}}
  if(selected.IsNull())throw std::runtime_error("hollow needs a planar end face");
  NCollection_List<TopoDS_Shape> open;open.Append(selected);BRepOffsetAPI_MakeThickSolid op;op.MakeThickSolidByJoin(s,open,-thickness,1e-4, BRepOffset_Skin,false,false,GeomAbs_Arc);op.Build();if(!op.IsDone())throw std::runtime_error("hollow failed; thickness may be too large");return checked(op.Shape(),"hollow");
}
TopoDS_Shape mirror(const TopoDS_Shape& s,char axis){gp_Trsf t;gp_Ax2 plane(gp_Pnt(0,0,0),axis=='x'?gp_Dir(1,0,0):axis=='y'?gp_Dir(0,1,0):gp_Dir(0,0,1));t.SetMirror(plane);return BRepBuilderAPI_Transform(s,t,true).Shape();}
TopoDS_Shape transform(const TopoDS_Shape& s,Vec3 t,Vec3 r,Vec3 scale){
  auto bnd=bounds(s);envelope(bnd);
  Shape display;display.translation=t;display.rotation=r;display.scale=scale;
  std::array<double,6> transformed{INFINITY,INFINITY,INFINITY,-INFINITY,-INFINITY,-INFINITY};
  for(int bits=0;bits<8;++bits){auto p=transform_point(display,{bnd[(bits&1)?3:0],bnd[(bits&2)?4:1],bnd[(bits&4)?5:2]});
    const double xyz[]={p.x,p.y,p.z};for(int j=0;j<3;++j){transformed[j]=std::min(transformed[j],xyz[j]);transformed[j+3]=std::max(transformed[j+3],xyz[j]);}}
  envelope(transformed);
  if(t.x==0&&t.y==0&&t.z==0&&r.x==0&&r.y==0&&r.z==0&&scale.x==1&&scale.y==1&&scale.z==1)return s;
  double cx=cos(r.x),sx=sin(r.x),cy=cos(r.y),sy=sin(r.y),cz=cos(r.z),sz=sin(r.z);
  const double a=cz*cy,b=cz*sy*sx-sz*cx,c=cz*sy*cx+sz*sx,d=sz*cy,e=sz*sy*sx+cz*cx,f=sz*sy*cx-cz*sx,g=-sy,h=cy*sx,i=cy*cx;
  if(scale.x==scale.y&&scale.x==scale.z){gp_Trsf tr;double q=scale.x;tr.SetValues(a*q,b*q,c*q,t.x,d*q,e*q,f*q,t.y,g*q,h*q,i*q,t.z);return BRepBuilderAPI_Transform(s,tr,true).Shape();}
  gp_Mat m(a*scale.x,b*scale.y,c*scale.z,d*scale.x,e*scale.y,f*scale.z,g*scale.x,h*scale.y,i*scale.z);gp_GTrsf gt;gt.SetVectorialPart(m);gt.SetTranslationPart(gp_XYZ(t.x,t.y,t.z));return BRepBuilderAPI_GTransform(s,gt,true).Shape();
}
TopoDS_Shape extrude(const Sketch& s,double height){dimension(height);require_sketch(s);BRepBuilderAPI_MakePolygon poly;for(const auto& p:s.points)poly.Add(gp_Pnt(p.X(),p.Y(),0));poly.Close();BRepBuilderAPI_MakeFace face(poly.Wire());if(!face.IsDone())throw std::runtime_error("sketch does not form a valid face");auto result=BRepPrimAPI_MakePrism(face.Face(),gp_Vec(0,0,height)).Shape();if(!BRepCheck_Analyzer(result).IsValid())throw std::runtime_error("extrusion produced an invalid solid");return checked(result,"extrusion");}
TopoDS_Shape revolve_y(const Sketch& s){require_sketch(s);BRepBuilderAPI_MakePolygon poly;for(const auto& p:s.points)poly.Add(gp_Pnt(p.X(),p.Y(),0));poly.Close();BRepBuilderAPI_MakeFace face(poly.Wire());if(!face.IsDone())throw std::runtime_error("profile does not form a valid face");auto result=BRepPrimAPI_MakeRevol(face.Face(),gp_Ax1(gp_Pnt(0,0,0),gp_Dir(0,1,0)),2*std::acos(-1.0)).Shape();if(!BRepCheck_Analyzer(result).IsValid())throw std::runtime_error("revolution produced an invalid solid");return checked(result,"revolution");}
double volume(const TopoDS_Shape& s){GProp_GProps p;BRepGProp::VolumeProperties(s,p);double v=p.Mass();if(!std::isfinite(v))throw std::runtime_error("volume is not finite");return v;}
std::array<double,6> bounds(const TopoDS_Shape& s){Bnd_Box b;BRepBndLib::AddOptimal(s,b);if(b.IsVoid())throw std::runtime_error("shape has empty bounds");double x0,y0,z0,x1,y1,z1;b.Get(x0,y0,z0,x1,y1,z1);return {x0,y0,z0,x1,y1,z1};}
Mesh tessellate(const TopoDS_Shape& shape,double tolerance){
  require_shape(shape,"tessellation");
  if(!std::isfinite(tolerance)||tolerance<0.01||tolerance>1.0)throw std::runtime_error("CAD tessellation deflection must be between 0.01 and 1");
  envelope(bounds(shape));
  size_t faces=0,edges=0;
  for(TopExp_Explorer e(shape,TopAbs_FACE);e.More();e.Next())if(++faces>2000)throw std::runtime_error("CAD face limit exceeded (2000)");
  for(TopExp_Explorer e(shape,TopAbs_EDGE);e.More();e.Next())if(++edges>10000)throw std::runtime_error("CAD edge limit exceeded (10000)");
  BRepMesh_IncrementalMesh mesher(shape,tolerance,false,0.35,false);if(!mesher.IsDone())throw std::runtime_error("BRepMesh tessellation failed");Mesh out;
  for(TopExp_Explorer e(shape,TopAbs_FACE);e.More();e.Next()){TopoDS_Face f=TopoDS::Face(e.Current());TopLoc_Location loc;Handle(Poly_Triangulation) tri=BRep_Tool::Triangulation(f,loc);if(tri.IsNull())continue;const gp_Trsf tr=loc.Transformation();for(int i=1;i<=tri->NbTriangles();++i){int a,b,c;tri->Triangle(i).Get(a,b,c);gp_Pnt pa=tri->Node(a).Transformed(tr),pb=tri->Node(b).Transformed(tr),pc=tri->Node(c).Transformed(tr);if(f.Orientation()==TopAbs_REVERSED)std::swap(pb,pc);if(out.triangles.size()>=1000000)throw std::runtime_error("CAD triangle limit exceeded (1000000)");out.triangles.push_back({{pa.X(),pa.Y(),pa.Z()},{pb.X(),pb.Y(),pb.Z()},{pc.X(),pc.Y(),pc.Z()}});}}
  if(out.triangles.empty())throw std::runtime_error("shape tessellated to no triangles");
  // OCCT may enumerate identical faces in different orders across processes.
  // Preserve winding, but canonicalize cyclic corners and triangle traversal
  // before accumulating normals or writing mesh bytes.
  auto point_less=[](Vec3 a,Vec3 b){return std::tie(a.x,a.y,a.z)<std::tie(b.x,b.y,b.z);};
  for(auto& t:out.triangles){
    if(point_less(t.b,t.a)&&point_less(t.b,t.c)){auto a=t.a;t.a=t.b;t.b=t.c;t.c=a;}
    else if(point_less(t.c,t.a)){auto a=t.a;t.a=t.c;t.c=t.b;t.b=a;}
  }
  std::sort(out.triangles.begin(),out.triangles.end(),[](const Triangle& a,const Triangle& b){
    return std::tie(a.a.x,a.a.y,a.a.z,a.b.x,a.b.y,a.b.z,a.c.x,a.c.y,a.c.z)<
           std::tie(b.a.x,b.a.y,b.a.z,b.b.x,b.b.y,b.b.z,b.c.x,b.c.y,b.c.z);
  });
  using Key=std::tuple<long long,long long,long long>;auto key=[](Vec3 p){constexpr double q=1e7;return Key{std::llround(p.x*q),std::llround(p.y*q),std::llround(p.z*q)};};
  std::map<Key,std::vector<Vec3>> adjacent;std::vector<Vec3> face_normals;face_normals.reserve(out.triangles.size());
  for(const auto& t:out.triangles){Vec3 n=normalized(cross(t.b-t.a,t.c-t.a));face_normals.push_back(n);for(Vec3 p:{t.a,t.b,t.c})adjacent[key(p)].push_back(n);}
  for(size_t j=0;j<out.triangles.size();++j){auto& t=out.triangles[j];auto smooth=[&](Vec3 p){Vec3 sum{};for(auto n:adjacent[key(p)])if(dot(n,face_normals[j])>=0.5)sum=sum+n;return normalized(sum);};t.na=smooth(t.a);t.nb=smooth(t.b);t.nc=smooth(t.c);t.smooth=true;}
  return out;
}
}
