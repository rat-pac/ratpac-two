#include "RAT/GroupVelocityCalculator.hh"

#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4MaterialPropertyVector.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "RAT/DB.hh"
#include "RAT/Log.hh"

namespace RAT {

GroupVelocityCalculator::Result RindexGroupVelocityCalculator::ComputeVelocity(const G4Material* material,
                                                                               G4double wavelength_nm,
                                                                               const G4ThreeVector& /*startPos*/,
                                                                               const G4ThreeVector& /*endPos*/) const {
  if (material == nullptr) {
    debug << "[RindexGroupVelocityCalculator] DEBUG: material is nullptr; using default n = " << fDefaultRindex
          << newline;
    return {.velocity = c_light / fDefaultRindex, .status = Status_t::kNormal};
  }
  std::string materialName = material->GetName();
  bool is_photocathode = false;
  try {
    is_photocathode = DB::Get()->GetLink("OPTICS", materialName)->GetZ("is_photocathode");
  } catch (DBNotFoundError) {
  }

  if (is_photocathode) {
    debug << "[RindexGroupVelocityCalculator] DEBUG: material " << materialName << " is a photocathode; skip track"
          << newline;
    return {.velocity = 0., .status = Status_t::kSkip};
  }

  G4double n = fDefaultRindex;
  Status_t status = Status_t::kNormal;
  G4MaterialPropertiesTable* mpt = material->GetMaterialPropertiesTable();

  if (mpt != nullptr) {
    G4MaterialPropertyVector* rindex = mpt->GetProperty("RINDEX");
    if (rindex != nullptr && rindex->GetVectorLength() > 0) {
      const G4double photonEnergy = (wavelength_nm > 0.) ? (h_Planck * c_light / (wavelength_nm * nm)) : 0.;
      n = rindex->Value(photonEnergy);
    } else {
      debug << "[RindexGroupVelocityCalculator] DEBUG: material " << material->GetName()
            << " has no RINDEX property; using default n = " << fDefaultRindex << newline;
    }
  }

  return {.velocity = c_light / n, .status = status};
}

}  // namespace RAT
