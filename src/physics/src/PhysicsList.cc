#include <Shielding.hh>
#include <stdexcept>
#include <string>
// Required for G4 > 10.5
#include <G4EmParameters.hh>
#include <G4FastSimulationManagerProcess.hh>
#include <G4HadronicInteractionRegistry.hh>
#include <G4HadronicProcess.hh>
#include <G4Neutron.hh>
#include <G4NeutronHPThermalScattering.hh>
#include <G4NeutronHPThermalScatteringData.hh>
#include <G4OpBoundaryProcess.hh>
#include <G4OpticalPhoton.hh>
#include <G4ParticleDefinition.hh>
#include <G4ParticleTypes.hh>
#include <G4ProcessManager.hh>
#include <G4RunManager.hh>
#include <RAT/BNLOpWLSBuilder.hh>
#include <RAT/DB.hh>
#include <RAT/G4OpWLSBuilder.hh>
#include <RAT/GLG4OpAttenuation.hh>
#include <RAT/GLG4Scint.hh>
#include <RAT/GLG4SteppingAction.hh>
#include <RAT/Log.hh>
#include <RAT/OpRayleigh.hh>
#include <RAT/PhotonThinning.hh>
#include <RAT/PhysicsList.hh>
#include <RAT/PhysicsListMessenger.hh>
#include <RAT/ThinnableG4Cerenkov.hh>

namespace RAT {

PhysicsList::PhysicsList() : Shielding(), wlsModel(nullptr) {
  DBLinkPtr physicsdb = DB::Get()->GetLink("PHYSICS");

  // Cherenkov process settings
  this->IsCerenkovEnabled = physicsdb->GetZ("enable_cerenkov");
  this->CerenkovMaxNumPhotonsPerStep = physicsdb->GetI("cerenkov_max_num_photons_per_step");

  // Step sizes for light ions (alpha), muons, and hadrons
  this->stepRatioLightIons = physicsdb->GetD("step_function_light_ions_d_over_R");
  this->finalRangeLightIons = physicsdb->GetD("step_function_light_ions_final_range");
  this->stepRatioMuHad = physicsdb->GetD("step_function_muon_hadron_d_over_R");
  this->finalRangeMuHad = physicsdb->GetD("step_function_muon_hadron_final_range");

  // WLS settings
  this->wlsModelName = physicsdb->GetS("wls_model_name");

  new PhysicsListMessenger(this);
}

PhysicsList::~PhysicsList() {}

void PhysicsList::ConstructParticle() {
  Shielding::ConstructParticle();
  G4OpticalPhoton::OpticalPhotonDefinition();
}

void PhysicsList::ConstructProcess() {
  G4EmParameters *param = G4EmParameters::Instance();
  param->SetStepFunctionLightIons(this->stepRatioLightIons, this->finalRangeLightIons);
  param->SetStepFunctionMuHad(this->stepRatioMuHad, this->finalRangeMuHad);

  AddParameterization();
  Shielding::ConstructProcess();
  ConstructOpticalProcesses();
  EnableThermalNeutronScattering();
}

void PhysicsList::EnableThermalNeutronScattering() {
  // Get the particle definition for neutrons
  G4ParticleDefinition *n_definition = G4Neutron::Definition();

  // Get the elastic scattering process used for neutrons
  G4HadronicProcess *n_elastic_process = nullptr;
  G4ProcessVector *proc_vec = n_definition->GetProcessManager()->GetProcessList();
  for (int i = 0; i < proc_vec->size(); i++) {
    G4VProcess *proc = proc_vec->operator[](i);
    if (proc->GetProcessSubType() == fHadronElastic && proc->GetProcessType() == fHadronic) {
      n_elastic_process = dynamic_cast<G4HadronicProcess *>(proc);
      break;
    }
  }
  if (!n_elastic_process) {
    warn << "PhysicsList::EnableThermalNeutronScattering: "
         << " couldn't find hadron elastic scattering process." << newline;
    throw std::runtime_error(std::string("Missing") + " hadron elastic" + " scattering process in PhysicsList");
  }

  // Get the "regular" neutron HP elastic scattering model
  G4HadronicInteraction *n_elastic_hp = G4HadronicInteractionRegistry::Instance()->FindModel("NeutronHPElastic");
  if (!n_elastic_hp) {
    warn << "PhysicsList::EnableThermalNeutronScattering: "
         << " couldn't find high-precision neutron elastic"
         << " scattering interaction." << newline;
    throw std::runtime_error(std::string("Missing") + " NeutronHPElastic" + " scattering interaction in PhysicsList");
  }

  // Exclude the thermal scattering region (below 4 eV) from the "regular"
  // elastic scattering model
  n_elastic_hp->SetMinEnergy(4. * CLHEP::eV);

  // Use the more detailed HP thermal scattering treatment below 4 eV instead
  n_elastic_process->RegisterMe(new G4NeutronHPThermalScattering);
  n_elastic_process->AddDataSet(new G4NeutronHPThermalScatteringData);
}

void PhysicsList::SetOpWLSModel(std::string model) {
  this->wlsModelName = model;
  delete this->wlsModel;

  if (model == "g4") {
    this->wlsModel = new G4OpWLSBuilder();
  } else if (model == "bnl") {
    this->wlsModel = new BNLOpWLSBuilder();
  } else if (model == "") {
    this->wlsModel = nullptr;
  } else {
    warn << "PhysicsList::SetOpWLSModel: Unknown model \"" << model << "\"" << newline;
    throw std::runtime_error("Unknown WLS model in PhysicsList");
  }

  info << "PhysicsList::SetOpWLSModel: Set WLS model to \"" << model << "\"" << newline;

  G4RunManager::GetRunManager()->PhysicsHasBeenModified();
}

void PhysicsList::ConstructOpticalProcesses() {
  // Cherenkov: G4Cerenkov with thinning and thresholding applied after-the-fact
  //
  // Request that Cerenkov photons be tracked first, before continuing
  // originating particle step.  Otherwise, we get too many secondaries!
  ThinnableG4Cerenkov *cerenkovProcess = nullptr;
  if (this->IsCerenkovEnabled) {
    cerenkovProcess = new ThinnableG4Cerenkov();
    double thinning = 1.0 / RAT::PhotonThinning::GetCherenkovThinningFactor();
    cerenkovProcess->SetThinningFactor(thinning);
    cerenkovProcess->SetLowerWavelengthThreshold(RAT::PhotonThinning::GetCherenkovLowerWavelengthThreshold());
    cerenkovProcess->SetUpperWavelengthThreshold(RAT::PhotonThinning::GetCherenkovUpperWavelengthThreshold());
    cerenkovProcess->SetTrackSecondariesFirst(true);
    cerenkovProcess->SetMaxNumPhotonsPerStep(this->CerenkovMaxNumPhotonsPerStep);
  }

  // Attenuation: RAT's GLG4OpAttenuation
  //
  // GLG4OpAttenuation implements Rayleigh scattering.
  GLG4OpAttenuation *attenuationProcess = new GLG4OpAttenuation();

  // Scintillation: RAT's GLG4Scint
  //
  // Create scintillation processes which depend on the mass.
  // Particles use following processes depending on mass:
  // - 0 < m < 0.9*proton (844 MeV) -- use default process (electron)
  // - 0.9*proton (844 MeV) < m < 0.999*neutron (938.6 MeV) -- use proton process
  // - 0.999*neutron (938.6 MeV) < m < 0.9*alpha (3.35 GeV) -- use neutron process
  // - m > 0.9*alpha (3.35 GeV) -- use alpha process
  G4double protonMass = G4Proton::Proton()->GetPDGMass();
  G4double neutronMass = G4Neutron::Neutron()->GetPDGMass();
  G4double alphaMass = G4Alpha::Alpha()->GetPDGMass();

  GLG4Scint *defaultScintProcess = new GLG4Scint();
  GLG4Scint *protonScintProcess = new GLG4Scint("proton", 0.9 * protonMass);
  GLG4Scint *neutronScintProcess = new GLG4Scint("neutron", 0.999 * neutronMass);
  GLG4Scint *alphaScintProcess = new GLG4Scint("alpha", 0.9 * alphaMass);

  // Optical boundary processes: default G4
  G4OpBoundaryProcess *opBoundaryProcess = new G4OpBoundaryProcess();
  // Rayleigh Scattering
  OpRayleigh *opRayleigh = new OpRayleigh();

  // Wavelength shifting: User-selectable via PhysicsListMessenger
  // Add call to SetOpWLSModel so it can be set from DB too... Need to reorganize in future
  SetOpWLSModel(this->wlsModelName);
  if (this->wlsModel) {
    wlsModel->ConstructProcess();
  }

  // Set verbosity
  if (verboseLevel > 0) {
    if (this->IsCerenkovEnabled) {
      cerenkovProcess->DumpInfo();
    }
    attenuationProcess->DumpInfo();
    defaultScintProcess->DumpInfo();
    protonScintProcess->DumpInfo();
    neutronScintProcess->DumpInfo();
    alphaScintProcess->DumpInfo();
    opBoundaryProcess->DumpInfo();
  }

  if (this->IsCerenkovEnabled) {
    cerenkovProcess->SetVerboseLevel(verboseLevel - 1);
  }
  attenuationProcess->SetVerboseLevel(verboseLevel - 1);
  defaultScintProcess->SetVerboseLevel(verboseLevel - 1);
  protonScintProcess->SetVerboseLevel(verboseLevel - 1);
  neutronScintProcess->SetVerboseLevel(verboseLevel - 1);
  alphaScintProcess->SetVerboseLevel(verboseLevel - 1);
  opBoundaryProcess->SetVerboseLevel(verboseLevel - 1);

  // Apply processes to all particles where applicable
  GetParticleIterator()->reset();
  while ((*GetParticleIterator())()) {
    G4ParticleDefinition *particle = GetParticleIterator()->value();
    G4ProcessManager *pmanager = particle->GetProcessManager();
    G4String particleName = particle->GetParticleName();
    if (this->IsCerenkovEnabled) {
      if (cerenkovProcess->IsApplicable(*particle)) {
        pmanager->AddProcess(cerenkovProcess);
        pmanager->SetProcessOrdering(cerenkovProcess, idxPostStep);
      }
    }
    if (particleName == "opticalphoton") {
      pmanager->AddDiscreteProcess(attenuationProcess);
      pmanager->AddDiscreteProcess(opBoundaryProcess);
      pmanager->AddDiscreteProcess(opRayleigh);
    }
  }
}

void PhysicsList::AddParameterization() {
  G4FastSimulationManagerProcess *fastSimulationManagerProcess = new G4FastSimulationManagerProcess();
  GetParticleIterator()->reset();
  while ((*GetParticleIterator())()) {
    G4ParticleDefinition *particle = GetParticleIterator()->value();
    G4ProcessManager *pmanager = particle->GetProcessManager();
    if (particle->GetParticleName() == "opticalphoton") {
      pmanager->AddProcess(fastSimulationManagerProcess, -1, -1, 1);
    }
  }
}

}  // namespace RAT
