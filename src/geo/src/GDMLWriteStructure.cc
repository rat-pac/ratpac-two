#include "RAT/GDMLWriteStructure.hh"

#include "CLHEP/Units/SystemOfUnits.h"
#include "G4Physics2DVector.hh"
#include "G4VSolid.hh"
#include "RAT/GLG4TorusStack.hh"

using namespace CLHEP;
namespace RAT {
void GDMLWriteStructure::AddSolid(const G4VSolid* const solidPtr) {
  if (solidPtr->GetEntityType() == "GLG4TorusStack") {
    for (std::size_t i = 0; i < solidList.size(); ++i)  // Check if solid is
    {                                                   // already in the list!
      if (solidList[i] == solidPtr) {
        return;
      }
    }
    solidList.push_back(solidPtr);
    const GLG4TorusStack* const torusStackPtr = static_cast<const GLG4TorusStack*>(solidPtr);
    GLG4TorusStackWrite(solidsElement, torusStackPtr);
  } else  // call default G4 implementation
    G4GDMLWriteSolids::AddSolid(solidPtr);
}

void GDMLWriteStructure::GLG4TorusStackWrite(xercesc::DOMElement* solElement, const GLG4TorusStack* const torusStack) {
  const G4String& name = GenerateName(torusStack->GetName(), torusStack);
  xercesc::DOMElement* torusStackElement = NewElement("torusstack");
  torusStackElement->setAttributeNode(NewAttribute("name", name));
  torusStackElement->setAttributeNode(NewAttribute("lunit", "mm"));
  int n_segments = torusStack->GetN();
  for (int i = 0; i < n_segments + 1; i++) {
    xercesc::DOMElement* edgeElement = NewElement("edge");
    edgeElement->setAttributeNode(NewAttribute("z", torusStack->GetZEdge(i) / mm));
    edgeElement->setAttributeNode(NewAttribute("rho", torusStack->GetRhoEdge(i) / mm));
    torusStackElement->appendChild(edgeElement);
    if (i < n_segments) {
      xercesc::DOMElement* originElement = NewElement("origin");
      originElement->setAttributeNode(NewAttribute("z", torusStack->GetZo(i) / mm));
      originElement->setAttributeNode(NewAttribute("rho", torusStack->GetRo(i) / mm));
      torusStackElement->appendChild(originElement);
    }
  }
  solElement->appendChild(torusStackElement);
  GLG4TorusStack* inner = torusStack->GetInner();
  if (inner) {
    xercesc::DOMElement* innerElement = NewElement("inner");
    GLG4TorusStackWrite(innerElement, inner);
    torusStackElement->appendChild(innerElement);
  }
}

void GDMLWriteStructure::OpticalSurfaceWrite(xercesc::DOMElement* solElement, const G4OpticalSurface* const surf) {
  xercesc::DOMElement* optElement = NewElement("opticalsurface");
  G4OpticalSurfaceModel smodel = surf->GetModel();
  G4double sval = (smodel == glisur) ? surf->GetPolish() : surf->GetSigmaAlpha();
  const G4String& name = GenerateName(surf->GetName(), surf);

  optElement->setAttributeNode(NewAttribute("name", name));
  optElement->setAttributeNode(NewAttribute("model", smodel));
  optElement->setAttributeNode(NewAttribute("finish", surf->GetFinish()));
  optElement->setAttributeNode(NewAttribute("type", surf->GetType()));
  optElement->setAttributeNode(NewAttribute("value", sval));

  // Write any property attached to the optical surface...
  //
  if (surf->GetMaterialPropertiesTable()) {
    PropertyWrite(optElement, surf);
  }
  const G4Physics2DVector* const dichroic_vector = const_cast<G4OpticalSurface*>(surf)->GetDichroicVector();
  xercesc::DOMElement* propElement;
  if (dichroic_vector) {
    propElement = NewElement("dichroic_data");
    size_t length_x = dichroic_vector->GetLengthX();
    size_t length_y = dichroic_vector->GetLengthY();
    propElement->setAttributeNode(NewAttribute("x_length", length_x));
    propElement->setAttributeNode(NewAttribute("y_length", length_y));
    xercesc::DOMElement* x_element = NewElement("x");
    xercesc::DOMElement* y_element = NewElement("y");
    std::ostringstream xvalues;
    std::ostringstream yvalues;
    for (size_t i = 0; i < length_x; i++) {
      if (i != 0) xvalues << " ";
      xvalues << dichroic_vector->GetX(i);
    }
    x_element->setAttributeNode(NewAttribute("values", xvalues.str()));
    for (size_t i = 0; i < length_y; i++) {
      if (i != 0) yvalues << " ";
      yvalues << dichroic_vector->GetY(i);
    }
    y_element->setAttributeNode(NewAttribute("values", yvalues.str()));
    propElement->appendChild(x_element);
    propElement->appendChild(y_element);
    xercesc::DOMElement* data = NewElement("data");
    std::ostringstream data_values;
    for (size_t i = 0; i < length_x; i++) {
      for (size_t j = 0; j < length_y; j++) {
        if (i != 0 || j != 0) data_values << " ";
        data_values << dichroic_vector->GetValue(i, j);
      }
    }
    data->setAttributeNode(NewAttribute("values", data_values.str()));
    propElement->appendChild(data);
    optElement->appendChild(propElement);
  }

  solElement->appendChild(optElement);
}

}  // namespace RAT
