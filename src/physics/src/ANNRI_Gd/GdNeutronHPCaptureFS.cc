///////////////////////////////////////////////////////////////////////////////
//                   Spectrum of radiative neutron capture by Gadolinium
//                                    version 1.0.0
//                                    (Sep.09.2005)

//                Author : karim.zbiri@subatech.in2p3.fr

// Modified class from original G4NeutronHPCaptureFS class to deexcite and
// add correctly the secondary to the hadronic final state

// Karim Zbiri, Aug, 2005
///////////////////////////////////////////////////////////////////////////////
#include "RAT/GdNeutronHPCaptureFS.hh"

#include <sstream>

#include "G4Fragment.hh"
#include "G4Gamma.hh"
#include "G4IonTable.hh"
#include "G4Nucleus.hh"
#include "G4ParticleHPDataUsed.hh"
#include "G4ParticleHPManager.hh"
#include "G4ParticleTable.hh"
#include "G4PhotonEvaporation.hh"
#include "G4PhysicalConstants.hh"
#include "G4PhysicsModelCatalog.hh"
#include "G4RandomDirection.hh"
#include "G4ReactionProduct.hh"
#include "G4SystemOfUnits.hh"
#include "RAT/ANNRIGd_GdNCaptureGammaGenerator.hh"
#include "RAT/ANNRIGd_GeneratorConfigurator.hh"
#include "RAT/ANNRIGd_OutputConverter.hh"
#include "fstream"
std::ofstream outf;

namespace AGd = ANNRIGdGammaSpecModel;

#define File_Name "GdNeutronHPCaptireFS.cc"

extern G4int    MODEL;      //1:garnet, 2:glg4sim, 4:ANNRI-Gd
extern G4int    Gd_CAPTURE; //1:natural , 2:enriched 157Gd, 3:enriched 155Gd
extern G4int    Gd_CASCADE; //1:discrete + continuum; 2:discrete, 3:continuum
extern G4String Gd157_ROOTFile;
extern G4String Gd155_ROOTFile;

/*
// When you choose "Gd target", which model do you use?
// 1:ggarnet, 2:glg4sim, 3:Geant4 default 4:ANNRI-Gd
G4int MODEL = 4;
// When you choose both "Gd target" and "ggarnet" or "glg4sim", you have to define the type of Gd.
// CAPTURE = 1:natural , 2:enriched 157Gd, 3:enriched 155Gd
// CASCADE = 1:discrete paek + continuum part, 2:discrete peaks, 3:continuum part
G4int Gd_CAPTURE = 1;
G4int Gd_CASCADE = 1;
// When you choose "Gd target", "ggarnet" and "continuum part", you have to define the continuum parameter.
// org:RIPL-3&&kopecky, yano_0908:newly yano tuning, yano_0909:use Iwamoto's parameter, ...
G4String Gd157_File = "cont_dat/Gd157_org.dat";  //"cont_dat/Gd157_yano_0909.dat";
G4String Gd157_ROOTFile = "cont_dat/158GdContTbl__E1SLO4__HFB.root";
G4String Gd155_File = "cont_dat/Gd155.dat";
G4String Gd155_ROOTFile = "cont_dat/156GdContTbl__E1SLO4__HFB.root";
*/

// use a static generator to avoid the construction and loading of data
// files for each Gd isotope
AGd::ANNRIGd_GdNCaptureGammaGenerator* GdNeutronHPCaptureFS::sAnnriGammaGen = 0;

////////////////////////////////////////////////////////////////////////////////////////
GdNeutronHPCaptureFS::GdNeutronHPCaptureFS()
////////////////////////////////////////////////////////////////////////////////////////
{
  secID = G4PhysicsModelCatalog::GetModelID("model_NeutronHPCapture_ANNRI");
  hasXsec = false;
  hasExactMF6 = false;
  targetMass = 0;
  if (MODEL == 4 and not sAnnriGammaGen) InitANNRIGdGenerator();
}

////////////////////////////////////////////////////////////////////////////////////////
G4HadFinalState* GdNeutronHPCaptureFS::ApplyYourself(const G4HadProjectile& theTrack)
///////////////////////////////////////////////////////////////////////////////////////
{
  if (theResult.Get() == nullptr) theResult.Put(new G4HadFinalState);
  
  theResult.Get()->Clear();
  G4int i;
  
  // prepare neutron
  G4double eKinetic = theTrack.GetKineticEnergy();
  const G4HadProjectile* incidentParticle = &theTrack;
  G4ReactionProduct theNeutron(theTrack.GetDefinition());
  theNeutron.SetMomentum(incidentParticle->Get4Momentum().vect());
  theNeutron.SetKineticEnergy(eKinetic);
  
  // Prepare target
  G4ReactionProduct theTarget;
  G4Nucleus aNucleus;
  if (targetMass < 500 * MeV)
    targetMass = G4NucleiProperties::GetNuclearMass(theBaseA, theBaseZ) / CLHEP::neutron_mass_c2;
  G4ThreeVector neutronVelocity = theNeutron.GetMomentum() / CLHEP::neutron_mass_c2;
  G4double temperature = theTrack.GetMaterial()->GetTemperature();
  theTarget = aNucleus.GetBiasedThermalNucleus(targetMass, neutronVelocity, temperature);
  G4ParticleDefinition* ion;
  theTarget.SetDefinitionAndUpdateE(ion = G4IonTable::GetIonTable()->GetIon(theBaseZ, theBaseA, 0.0));

// (begin) this block will be remove in next commit/ 
  const G4Material* theMaterial = theTrack.GetMaterial();
  std::size_t index = theMaterial->GetElement(0)->GetIndex();
  const G4Element* target_element = (*G4Element::GetElementTable())[index];  
  //G4cout << "Target Material of FinalState is " << theMaterial->GetName() << G4endl;
  //G4cout << "Target Element of FinalState is " << target_element->GetName() << G4endl;
// (end) this block will be remove in next commit

  // Put neutron in nucleus rest system
  theNeutron.Lorentz(theNeutron, theTarget);
  eKinetic = theNeutron.GetKineticEnergy();

  ///////////////////dice the photons////////////////////
  G4ReactionProductVector* thePhotons = nullptr;
  G4ParticleHPManager* hpmanager = G4ParticleHPManager::GetInstance();
  //G4cout << "Gd_CAPTURE (Gd155 = 3 || Gd157 = 2 // CaptureFS):" << Gd_CAPTURE << G4endl;
  thePhotons = GenerateWithANNRIGdGenerator();
        //G4cout << "thePhotons = GenerateWithANNRIGdGenerator();" <<G4endl;//this line will be remove in next commit
  // Add them to the final state
  G4int nPhotons = (G4int)thePhotons->size();
        //G4cout << "Check thePhotons->size() : " << nPhotons << G4endl;//this line will be remove in next commit
  if (!hpmanager->GetDoNotAdjustFinalState()) {
    // Make at least one photon
    // 101203 TK
    if (nPhotons == 0) {
      auto theOne = new G4ReactionProduct;
      theOne->SetDefinition(G4Gamma::Gamma());
      G4ThreeVector direction = G4RandomDirection();
      theOne->SetMomentum(direction);
      thePhotons->push_back(theOne);
      ++nPhotons;  // 0 -> 1
    }
    // One photon case: energy set to Q-value
    // 101203 TK
    if (nPhotons == 1 && (*thePhotons)[0]->GetDefinition()->GetBaryonNumber() == 0) {
      G4ThreeVector direction = (*thePhotons)[0]->GetMomentum().unit();
      G4double Q = G4IonTable::GetIonTable()->GetIonMass(theBaseZ, theBaseA, 0.0) + CLHEP::neutron_mass_c2 -
                   G4IonTable::GetIonTable()->GetIonMass(theBaseZ, theBaseA + 1, 0.0);
      (*thePhotons)[0]->SetMomentum(Q * direction);
    }
  }

  // back to lab system
  for (i = 0; i < nPhotons; i++) 
  {
    (*thePhotons)[i]->Lorentz(*((*thePhotons)[i]), -1 * theTarget);
  }

  // Recoil, if only one gamma
  // if (1==nPhotons)
  if (nPhotons == 1 && thePhotons->operator[](0)->GetDefinition()->GetBaryonNumber() == 0) {
    auto theOne = new G4DynamicParticle;
    G4ParticleDefinition* aRecoil = G4IonTable::GetIonTable()->GetIon(theBaseZ, theBaseA + 1, 0.0);
    theOne->SetDefinition(aRecoil);
    // Now energy;
    // Can be done slightly better @
    G4ThreeVector aMomentum =
        theTrack.Get4Momentum().vect() + theTarget.GetMomentum() - thePhotons->operator[](0)->GetMomentum();
    theOne->SetMomentum(aMomentum);
    theResult.Get()->AddSecondary(theOne, secID);
  }
  
  // Now fill in the gammas.
  for (i = 0; i < nPhotons; ++i) {
    // back to lab system
    auto theOne = new G4DynamicParticle;
    theOne->SetDefinition(thePhotons->operator[](i)->GetDefinition());
    theOne->SetMomentum(thePhotons->operator[](i)->GetMomentum());
    theResult.Get()->AddSecondary(theOne, secID);
    // (begin) this block will be remove in next commit
    /*
    if (thePhotons->operator[](i)->GetDefinition() == G4Gamma::Gamma())
        {
          G4cout << "thePhotons->operator[]("<<i<<")->GetDefinition() is Gamma " << G4endl;
        }
    if (thePhotons->operator[](i)->GetDefinition() == G4Electron::Electron())
        {
          G4cout << "thePhotons->operator[]("<<i<<")->GetDefinition() is Internal Conversion Electron " << G4endl;
        }
        G4cout << "thePhotons->operator[]("<<i<<")->GetMomentum() : " << thePhotons->operator[](i)->GetMomentum() << G4endl;
    */
    // (end) this block will be remove in next commit
    delete thePhotons->operator[](i);
  }
  delete thePhotons;

  // 101203TK
  G4bool residual = false;
  G4ParticleDefinition* aRecoil = G4IonTable::GetIonTable()->GetIon(theBaseZ, theBaseA + 1, 0.0);
  for (std::size_t j = 0; j != theResult.Get()->GetNumberOfSecondaries(); j++) {
    if (theResult.Get()->GetSecondary(j)->GetParticle()->GetDefinition() == aRecoil) residual = true;
  }

  if (!residual) {
    G4int nNonZero = 0;
    G4LorentzVector p_photons(0, 0, 0, 0);
    for (std::size_t j = 0; j != theResult.Get()->GetNumberOfSecondaries(); ++j) {
      p_photons += theResult.Get()->GetSecondary(j)->GetParticle()->Get4Momentum();
      // To many 0 momentum photons -> Check PhotonDist
      if (theResult.Get()->GetSecondary(j)->GetParticle()->Get4Momentum().e() > 0) nNonZero++;
    }

    // Can we include kinetic energy here?
    G4double deltaE =
        (theTrack.Get4Momentum().e() + theTarget.GetTotalEnergy()) - (p_photons.e() + aRecoil->GetPDGMass());

    // Add photons
    if (nPhotons - nNonZero > 0) {
      // G4cout << "TKDB G4NeutronHPCaptureFS::ApplyYourself we will create additional " <<
      // nPhotons - nNonZero << " photons" << G4endl;
      std::vector<G4double> vRand;
      vRand.push_back(0.0);
      for (G4int j = 0; j != nPhotons - nNonZero - 1; j++) {
        vRand.push_back(G4UniformRand());
      }
      vRand.push_back(1.0);
      std::sort(vRand.begin(), vRand.end());

      std::vector<G4double> vEPhoton;
      for (G4int j = 0; j < (G4int)vRand.size() - 1; j++) {
        vEPhoton.push_back(deltaE * (vRand[j + 1] - vRand[j]));
      }
      std::sort(vEPhoton.begin(), vEPhoton.end());

      for (G4int j = 0; j < nPhotons - nNonZero - 1; j++) {
        // Isotopic in LAB OK?
        //  Bug # 1745 DHW G4double theta = pi*G4UniformRand();
        G4ThreeVector tempVector = G4RandomDirection() * vEPhoton[j];

        p_photons += G4LorentzVector(tempVector, tempVector.mag());
        auto theOne = new G4DynamicParticle;
        theOne->SetDefinition(G4Gamma::Gamma());
        theOne->SetMomentum(tempVector);
        theResult.Get()->AddSecondary(theOne, secID);
      }

      //        Add last photon
      auto theOne = new G4DynamicParticle;
      theOne->SetDefinition(G4Gamma::Gamma());
      //        For better momentum conservation
      G4ThreeVector lastPhoton = -p_photons.vect().unit() * vEPhoton.back();
      p_photons += G4LorentzVector(lastPhoton, lastPhoton.mag());
      theOne->SetMomentum(lastPhoton);
      theResult.Get()->AddSecondary(theOne, secID);
    }

    // Add residual
    auto theOne = new G4DynamicParticle;
    G4ThreeVector aMomentum = theTrack.Get4Momentum().vect() + theTarget.GetMomentum() - p_photons.vect();
    theOne->SetDefinition(aRecoil);
    theOne->SetMomentum(aMomentum);
    theResult.Get()->AddSecondary(theOne, secID);
  }
  // 101203TK END

  // clean up the primary neutron
  theResult.Get()->SetStatusChange(stopAndKill);
  return theResult.Get();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
#include <sstream>
void GdNeutronHPCaptureFS::Init(G4double AA, G4double ZZ, G4int M, G4String& dirName, G4String&, G4ParticleDefinition*)
///////////////////////////////////////////////////////////////////////////////////////////////////
{
  G4int Z = G4lrint(ZZ);
  G4int A = G4lrint(AA);
  // TK110430 BEGIN
  std::stringstream ss;
  ss << Z;
  G4String sZ;
  ss >> sZ;
  ss.clear();
  ss << A;
  G4String sA;
  ss >> sA;

  ss.clear();
  G4String sM;
  if (M > 0) {
    ss << "m";
    ss << M;
    ss >> sM;
    ss.clear();
  }

  G4String element_name = theNames.GetName(Z - 1);
  G4String filenameMF6 = dirName + "/FSMF6/" + sZ + "_" + sA + sM + "_" + element_name;

  std::istringstream theData(std::ios::in);
  G4ParticleHPManager::GetInstance()->GetDataStream(filenameMF6, theData);

  // TK110430 Only use MF6MT102 which has exactly same A and Z
  // Even _nat_ do not select and there is no _nat_ case in ENDF-VII.0
  if (theData.good()) {
    hasExactMF6 = true;
    theMF6FinalState.Init(theData);
    return;
  }
  // TK110430 END

  G4String tString = "/FS";
  G4bool dbool;
  G4ParticleHPDataUsed aFile = theNames.GetName(A, Z, M, dirName, tString, dbool);

  G4String filename = aFile.GetName();
  SetAZMs(A, Z, M, aFile);
  if (!dbool || (Z <= 2 && (theBaseZ != Z || theBaseA != A))) {
    hasAnyData = false;
    hasFSData = false;
    hasXsec = false;
    return;
  }
  theData.clear();
  G4ParticleHPManager::GetInstance()->GetDataStream(filename, theData);
  hasFSData = theFinalStatePhotons.InitMean(theData);
  if (hasFSData) {
    targetMass = theFinalStatePhotons.GetTargetMass();
    theFinalStatePhotons.InitAngular(theData);
    theFinalStatePhotons.InitEnergies(theData);
  }
}

////////////////////////////////////////////////////////////////////////////////
void GdNeutronHPCaptureFS::InitANNRIGdGenerator()
////////////////////////////////////////////////////////////////////////////////
{
  if (sAnnriGammaGen) delete sAnnriGammaGen;
  sAnnriGammaGen = new AGd::ANNRIGd_GdNCaptureGammaGenerator();

  AGd::ANNRIGd_GeneratorConfigurator::Configure(*sAnnriGammaGen, Gd_CAPTURE, Gd_CASCADE, Gd155_ROOTFile,
                                                Gd157_ROOTFile);
}

////////////////////////////////////////////////////////////////////////////////
G4ReactionProductVector* GdNeutronHPCaptureFS::GenerateWithANNRIGdGenerator()
////////////////////////////////////////////////////////////////////////////////
{
  G4ReactionProductVector* theProducts = 0;

  if (sAnnriGammaGen) {
    AGd::ReactionProductVector products;
    if (Gd_CAPTURE == 1)  // nat. Gd
      products = sAnnriGammaGen->Generate_NatGd();
    else if (Gd_CAPTURE == 2 and Gd_CASCADE == 1)  // 157Gd, discrete + continuum
      products = sAnnriGammaGen->Generate_158Gd();
    else if (Gd_CAPTURE == 2 and Gd_CASCADE == 2)  // 157Gd, discrete
      products = sAnnriGammaGen->Generate_158Gd_Discrete();
    else if (Gd_CAPTURE == 2 and Gd_CASCADE == 3)  // 157Gd, continuum
      products = sAnnriGammaGen->Generate_158Gd_Continuum();
    else if (Gd_CAPTURE == 3 and Gd_CASCADE == 1)  // 155Gd, discrete + continuum
      products = sAnnriGammaGen->Generate_156Gd();
    else if (Gd_CAPTURE == 3 and Gd_CASCADE == 2)  // 155Gd, discrete
      products = sAnnriGammaGen->Generate_156Gd_Discrete();
    else if (Gd_CAPTURE == 3 and Gd_CASCADE == 3)  // 155Gd, continuum
      products = sAnnriGammaGen->Generate_156Gd_Continuum();

    theProducts = AGd::ANNRIGd_OutputConverter::ConvertToG4(products);
  } else {
    // Printing.Error(File_Name,"sAnnriGammaGen");
    theProducts = new G4ReactionProductVector();
  }

  return theProducts;
}
