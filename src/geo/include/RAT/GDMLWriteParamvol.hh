#ifndef GDMLWriteParamvol_hh
#define GDMLWriteParamvol_hh

#include "RAT/GDMLWriteSetup.hh"

class G4Box;
class G4Trd;
class G4Trap;
class G4Tubs;
class G4Cons;
class G4Sphere;
class G4Orb;
class G4Torus;
class G4Ellipsoid;
class G4Para;
class G4Hype;
class G4Polycone;
class G4Polyhedra;
class G4VPhysicalVolume;

namespace RAT {

class GDMLWriteParamvol : public GDMLWriteSetup {
 public:
  virtual void ParamvolWrite(xercesc::DOMElement*, const G4VPhysicalVolume* const);
  virtual void ParamvolAlgorithmWrite(xercesc::DOMElement* paramvolElement, const G4VPhysicalVolume* const paramvol);

 protected:
  GDMLWriteParamvol();
  virtual ~GDMLWriteParamvol();

  void Box_dimensionsWrite(xercesc::DOMElement*, const G4Box* const);
  void Trd_dimensionsWrite(xercesc::DOMElement*, const G4Trd* const);
  void Trap_dimensionsWrite(xercesc::DOMElement*, const G4Trap* const);
  void Tube_dimensionsWrite(xercesc::DOMElement*, const G4Tubs* const);
  void Cone_dimensionsWrite(xercesc::DOMElement*, const G4Cons* const);
  void Sphere_dimensionsWrite(xercesc::DOMElement*, const G4Sphere* const);
  void Orb_dimensionsWrite(xercesc::DOMElement*, const G4Orb* const);
  void Torus_dimensionsWrite(xercesc::DOMElement*, const G4Torus* const);
  void Ellipsoid_dimensionsWrite(xercesc::DOMElement*, const G4Ellipsoid* const);
  void Para_dimensionsWrite(xercesc::DOMElement*, const G4Para* const);
  void Hype_dimensionsWrite(xercesc::DOMElement*, const G4Hype* const);
  void Polycone_dimensionsWrite(xercesc::DOMElement*, const G4Polycone* const);
  void Polyhedra_dimensionsWrite(xercesc::DOMElement*, const G4Polyhedra* const);
  void ParametersWrite(xercesc::DOMElement*, const G4VPhysicalVolume* const, const G4int&);
};

}  // namespace RAT
#endif
