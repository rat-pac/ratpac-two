#ifndef GDMLWriteStructure_hh
#define GDMLWriteStructure_hh

#include "G4GDMLWriteStructure.hh"
#include "G4OpticalSurface.hh"
#include "GLG4TorusStack.hh"

namespace RAT {
class GDMLWriteStructure : public G4GDMLWriteStructure {
 public:
  virtual void AddSolid(const G4VSolid* const) override;

 protected:
  void GLG4TorusStackWrite(xercesc::DOMElement* solElement, const GLG4TorusStack* const torusStack);
  void OpticalSurfaceWrite(xercesc::DOMElement* solElement, const G4OpticalSurface* const surf);
};

}  // namespace RAT
#endif
