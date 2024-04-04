#include "RAT/GDMLWriteSetup.hh"

#include "G4LogicalVolume.hh"

namespace RAT {
// --------------------------------------------------------------------
GDMLWriteSetup::GDMLWriteSetup() : GDMLWriteSolids() {}

// --------------------------------------------------------------------
GDMLWriteSetup::~GDMLWriteSetup() {}

// --------------------------------------------------------------------
void GDMLWriteSetup::SetupWrite(xercesc::DOMElement* gdmlElement, const G4LogicalVolume* const logvol) {
#ifdef G4VERBOSE
  G4cout << "GDML: Writing setup..." << G4endl;
#endif
  const G4String worldref = GenerateName(logvol->GetName(), logvol);

  xercesc::DOMElement* setupElement = NewElement("setup");
  setupElement->setAttributeNode(NewAttribute("version", "1.0"));
  setupElement->setAttributeNode(NewAttribute("name", "Default"));
  xercesc::DOMElement* worldElement = NewElement("world");
  worldElement->setAttributeNode(NewAttribute("ref", worldref));
  setupElement->appendChild(worldElement);
  gdmlElement->appendChild(setupElement);
}
}  // namespace RAT
