#ifndef GDMLWriteSolids_hh
#define GDMLWriteSolids_hh

#include "G4GDMLWriteMaterials.hh"
#include "G4MultiUnion.hh"
#include "G4Types.hh"

class G4BooleanSolid;
class G4ScaledSolid;
class G4Box;
class G4Cons;
class G4EllipticalCone;
class G4Ellipsoid;
class G4EllipticalTube;
class G4ExtrudedSolid;
class G4Hype;
class G4Orb;
class G4Para;
class G4Paraboloid;
class G4Polycone;
class G4GenericPolycone;
class G4Polyhedra;
class G4Sphere;
class G4TessellatedSolid;
class G4Tet;
class G4Torus;
class G4GenericTrap;
class G4Trap;
class G4Trd;
class G4Tubs;
class G4CutTubs;
class G4TwistedBox;
class G4TwistedTrap;
class G4TwistedTrd;
class G4TwistedTubs;
class G4VSolid;
class G4OpticalSurface;
class GLG4TorusStack;

namespace RAT {

class GDMLWriteSolids : public G4GDMLWriteMaterials {
  class G4ThreeVectorCompare {
   public:
    G4bool operator()(const G4ThreeVector& t1, const G4ThreeVector& t2) const {
      if (t1.x() < t2.x()) return true;

      if (t1.y() < t2.y()) return true;

      if (t1.z() < t2.z()) return true;

      return false;
    }
  };

 public:
  virtual void AddSolid(const G4VSolid* const);
  virtual void SolidsWrite(xercesc::DOMElement*);

 protected:
  GDMLWriteSolids();
  virtual ~GDMLWriteSolids();

  void MultiUnionWrite(xercesc::DOMElement* solElement, const G4MultiUnion* const);
  void BooleanWrite(xercesc::DOMElement*, const G4BooleanSolid* const);
  void ScaledWrite(xercesc::DOMElement*, const G4ScaledSolid* const);
  void BoxWrite(xercesc::DOMElement*, const G4Box* const);
  void ConeWrite(xercesc::DOMElement*, const G4Cons* const);
  void ElconeWrite(xercesc::DOMElement*, const G4EllipticalCone* const);
  void EllipsoidWrite(xercesc::DOMElement*, const G4Ellipsoid* const);
  void EltubeWrite(xercesc::DOMElement*, const G4EllipticalTube* const);
  void XtruWrite(xercesc::DOMElement*, const G4ExtrudedSolid* const);
  void HypeWrite(xercesc::DOMElement*, const G4Hype* const);
  void OrbWrite(xercesc::DOMElement*, const G4Orb* const);
  void ParaWrite(xercesc::DOMElement*, const G4Para* const);
  void ParaboloidWrite(xercesc::DOMElement*, const G4Paraboloid* const);
  void PolyconeWrite(xercesc::DOMElement*, const G4Polycone* const);
  void GenericPolyconeWrite(xercesc::DOMElement*, const G4GenericPolycone* const);
  void PolyhedraWrite(xercesc::DOMElement*, const G4Polyhedra* const);
  void SphereWrite(xercesc::DOMElement*, const G4Sphere* const);
  void TessellatedWrite(xercesc::DOMElement*, const G4TessellatedSolid* const);
  void TetWrite(xercesc::DOMElement*, const G4Tet* const);
  void TorusWrite(xercesc::DOMElement*, const G4Torus* const);
  void GenTrapWrite(xercesc::DOMElement*, const G4GenericTrap* const);
  void TrapWrite(xercesc::DOMElement*, const G4Trap* const);
  void TrdWrite(xercesc::DOMElement*, const G4Trd* const);
  void TubeWrite(xercesc::DOMElement*, const G4Tubs* const);
  void CutTubeWrite(xercesc::DOMElement*, const G4CutTubs* const);
  void TwistedboxWrite(xercesc::DOMElement*, const G4TwistedBox* const);
  void TwistedtrapWrite(xercesc::DOMElement*, const G4TwistedTrap* const);
  void TwistedtrdWrite(xercesc::DOMElement*, const G4TwistedTrd* const);
  void TwistedtubsWrite(xercesc::DOMElement*, const G4TwistedTubs* const);
  void GLG4TorusStackWrite(xercesc::DOMElement*, const GLG4TorusStack* const);
  void ZplaneWrite(xercesc::DOMElement*, const G4double&, const G4double&, const G4double&);
  void RZPointWrite(xercesc::DOMElement*, const G4double&, const G4double&);
  void OpticalSurfaceWrite(xercesc::DOMElement*, const G4OpticalSurface* const);
  void PropertyWrite(xercesc::DOMElement*, const G4OpticalSurface* const);

 protected:
  std::vector<const G4VSolid*> solidList;
  xercesc::DOMElement* solidsElement = nullptr;
  static const G4int maxTransforms = 8;  // Constant for limiting the number
                                         // of displacements/reflections
                                         // applied to a single solid
};
}  // namespace RAT
#endif
