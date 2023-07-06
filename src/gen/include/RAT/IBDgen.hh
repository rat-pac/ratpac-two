#ifndef __RAT_IBDgen__
#define __RAT_IBDgen__

#include <CLHEP/Random/Randomize.h>

#include <G4LorentzVector.hh>
#include <G4String.hh>
#include <G4ThreeVector.hh>
#include <RAT/LinearInterp.hh>

namespace RAT {

class IBDgenMessenger;
// Generate inverse beta decay event
class IBDgen {
 public:
  IBDgen();

  // Generate random event vectors
  //    Pass in the neutrino direction (unit vector)
  //    Returns 4-momentum vectors for neutrino and resulting positron
  //    and neutron.  Neutrino energy and positron direction drawn from
  //    GenInteraction() distribution.
  void GenEvent(const G4ThreeVector &nu_dir, G4LorentzVector &neutrino, G4LorentzVector &positron,
                G4LorentzVector &neutron);

  // Generate random inverse beta decay interaction
  //
  //   Selects neutrino energy and cos(theta) of the produced
  //   positron relative to neutrino direction.  They are pulled
  //   from 2D distribution of reactor neutrinos which have interacted
  //   with a proton, so both the incident flux, and the (relative)
  //   differential cross-section are factored in.
  void GenInteraction(double &E, double &CosThetaLab);

  // Differential cross section for inverse beta decay
  static double CrossSection(double Enu, double CosThetaLab);
  // dE/dCosT for inverse beta decay (E = first order positron energy)
  static double dE1dCosT(double Enu, double CosThetaLab);
  // Maximum of dE/dCosT
  static double EvalMax(double Enu, double FluxMax);

  // Positron energy (first order)
  static double PositronEnergy(double Enu, double CosThetaLab);

  // Flux as a function of energy.  Interpolated from table in IBD RATDB table
  double Flux(double E) const { return rmpflux(E); };

  // Spectrum index for ratdb
  G4String GetSpectrumIndex() { return SpectrumIndex; };
  void SetSpectrumIndex(G4String _specIndex);
  // Enable neutrons
  bool GetNeutronState() { return NeutronState; };
  void SetNeutronState(bool _state) { NeutronState = _state; };
  // Enable positrons
  bool GetPositronState() { return PositronState; };
  void SetPositronState(bool _state) { PositronState = _state; };

  void UpdateFromDatabaseIndex();

 protected:
  LinearInterp<double> rmpflux;
  double Emax;
  double Emin;
  double XCmax;
  double FluxMax;

  bool NeutronState;
  bool PositronState;
  bool ApplyCrossSection;

  IBDgenMessenger *messenger;
  G4String SpectrumIndex;
};

}  // namespace RAT

#endif
