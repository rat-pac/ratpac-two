///////////////////////////////////////////////////////////////////////////////
//                    Spectrum of radiative neutron capture by Gadolinium
//                                    version 1.0.0
//                                    (Sep.09.2005)

//                Author : karim.zbiri@subatech.in2p3.fr

// Modified class from original G4NeutronHPCaptureFS class to deexcite and
// add correctly the secondary to the hadronic final state

// Karim Zbiri, Aug, 2005
///////////////////////////////////////////////////////////////////////////////

#ifndef GdNeutronHPCaptureFS_h
#define GdNeutronHPCaptureFS_h 1

#include "G4Fragment.hh"
#include "G4HadFinalState.hh"
#include "G4HadProjectile.hh"
#include "G4Nucleus.hh"
#include "G4ParticleHPEnAngCorrelation.hh"
#include "G4ParticleHPFinalState.hh"
#include "G4ParticleHPNames.hh"
#include "G4ParticleHPPhotonDist.hh"
#include "G4ReactionProductVector.hh"
#include "globals.hh"

//#include "GdCaptureGammas_ggarnet.hh"
//#include "GdCaptureGammas_glg4sim.hh"
//#include "DrawMessage.hh"

// Forward declaration
namespace ANNRIGdGammaSpecModel {
class ANNRIGd_GdNCaptureGammaGenerator;
}
namespace AGd = ANNRIGdGammaSpecModel;

class GdNeutronHPCaptureFS : public G4ParticleHPFinalState {
 public:
  GdNeutronHPCaptureFS();
  ~GdNeutronHPCaptureFS() override = default;

  void Init(G4double A, G4double Z, G4int M, G4String& dirName, G4String& aFSType, G4ParticleDefinition*);
  G4HadFinalState* ApplyYourself(const G4HadProjectile& theTrack) override;
  G4ParticleHPFinalState* New() override {
    GdNeutronHPCaptureFS* theNew = new GdNeutronHPCaptureFS;
    return theNew;
  }
  GdNeutronHPCaptureFS(GdNeutronHPCaptureFS&) = delete;
  GdNeutronHPCaptureFS& operator=(const GdNeutronHPCaptureFS& right) = delete;

 private:
  void InitANNRIGdGenerator();
  G4ReactionProductVector* GenerateWithANNRIGdGenerator();

  G4double targetMass;
  G4bool hasExactMF6;

  G4ParticleHPPhotonDist theFinalStatePhotons;
  G4ParticleHPEnAngCorrelation theMF6FinalState;
  G4ParticleHPNames theNames;

  // GdCaptureGammas_ggarnet theFinalgammas_ggarnet;
  // GdCaptureGammas_glg4sim theFinalgammas_glg4sim;

  // DrawMessage Printing;

 private:
  static AGd::ANNRIGd_GdNCaptureGammaGenerator* sAnnriGammaGen;
};
#endif
