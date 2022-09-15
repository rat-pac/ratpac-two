/**
 * @class RAT::PhysicsList
 * @brief Defines the physics processes active in the simulation
 *
 * @author A. Mastbaum <mastbaum@hep.upenn.edu>
 *
 * @detail This physics list extends the Shielding list included with Geant4
 * to add optical processes and custom processes overridden in RAT.
 */

#ifndef __RAT_PhysicsList__
#define __RAT_PhysicsList__

#include <G4VUserPhysicsList.hh>
#include <Shielding.hh>
#include <string>

namespace RAT {

class PhysicsList : public Shielding {
 public:
  PhysicsList();

  ~PhysicsList();

  // Instantiate desired Particles
  void ConstructParticle();

  // Instantiate desired Processes
  void ConstructProcess();

  // Set the WLS model by name
  void SetOpWLSModel(std::string model);

  // Get the WLS model name
  std::string GetOpWLSModelName() { return this->wlsModelName; }

  void SetCerenkovMaxNumPhotonsPerStep(int maxphotons) { this->CerenkovMaxNumPhotonsPerStep = maxphotons; }
  int GetCerenkovMaxNumPhotonsPerStep() { return this->CerenkovMaxNumPhotonsPerStep; }

  void EnableCerenkov(bool status) { this->IsCerenkovEnabled = status; }
  bool GetCerenkovStatus() { return this->IsCerenkovEnabled; }

  void SetStepFunctionLightIons(double v1, double v2) {
    this->stepRatioLightIons = v1;
    this->finalRangeLightIons = v2;
  }
  void SetStepFunctionMuHad(double v1, double v2) {
    this->stepRatioMuHad = v1;
    this->finalRangeMuHad = v2;
  }

 private:
  // Construct and register optical processes
  void ConstructOpticalProcesses();

  void EnableThermalNeutronScattering();
  // Register opticalphotons with the PMT G4FastSimulationManagerProcess
  void AddParameterization();

  std::string wlsModelName;          // The name of the WLS model
  G4VPhysicsConstructor *wlsModel;   // The WLS model constructor
  int CerenkovMaxNumPhotonsPerStep;  // Controlls step-size for cerenkov
                                     // processes
  bool IsCerenkovEnabled;
  double stepRatioLightIons;
  double finalRangeLightIons;
  double stepRatioMuHad;
  double finalRangeMuHad;
};

}  // namespace RAT

#endif  // __RAT_PhysicsList__
