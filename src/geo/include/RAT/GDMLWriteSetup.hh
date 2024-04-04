#ifndef GDMLWriteSetup_hh
#define GDMLWriteSetup_hh

#include "RAT/GDMLWriteSolids.hh"

namespace RAT {
class GDMLWriteSetup : public GDMLWriteSolids {
 public:
  virtual void SetupWrite(xercesc::DOMElement*, const G4LogicalVolume* const);

 protected:
  GDMLWriteSetup();
  virtual ~GDMLWriteSetup();
};
}  // namespace RAT
#endif
