#include <CLHEP/Units/PhysicalConstants.h>

#include <G4Event.hh>
#include <G4IonTable.hh>
#include <G4ParticleTable.hh>
#include <G4PrimaryParticle.hh>
#include <G4PrimaryVertex.hh>
#include <G4ThreeVector.hh>
#include <RAT/IsotopeMessenger.hh>
#include <RAT/Log.hh>
#include <RAT/VertexGen_Isotope.hh>
#include <Randomize.hh>

#define std std
namespace RAT {

VertexGen_Isotope::VertexGen_Isotope(const char *arg_dbname) : GLG4VertexGen(arg_dbname) {
  messenger = new IsotopeMessenger(this);

  // constructor
  //        ADEFAULT = 16;
  //        ZDEFAULT = 8;
  //        EDEFAULT = 0;
}

///-------------------------------------------------------------------------
VertexGen_Isotope::~VertexGen_Isotope() {
  // destructor
}

///-------------------------------------------------------------------------
void VertexGen_Isotope::GeneratePrimaryVertex(G4Event *event, G4ThreeVector &dx, G4double dt) {
  // Where the main work is done - the astute may notice a strong similarity to
  // the Gun generator!

  G4PrimaryVertex *vertex = new G4PrimaryVertex(dx, dt);

  Double_t A = GetIsotopeA();
  Double_t Z = GetIsotopeZ();
  Double_t E = GetIsotopeE();
  G4ParticleDefinition *fIsotope = G4IonTable::GetIonTable()->GetIon(Z, A, E);

  G4PrimaryParticle *n_particle = new G4PrimaryParticle(fIsotope,  // particle code
                                                        0.0,       // x component of momentum
                                                        0.0,       // y component of momentum
                                                        0.0);      // z component of momentum
  n_particle->SetMass(fIsotope->GetPDGMass());                     // Geant4 is silly.
  vertex->SetPrimary(n_particle);

  event->AddPrimaryVertex(vertex);
}

///-------------------------------------------------------------------------
void VertexGen_Isotope::SetState(G4String newValues) {
  if (newValues.length() == 0) {
    // print help and current state
    info << "Current state of this VertexGen_Isotope:" << newline << " \"" << GetState() << "\"" << newline << newline;
    info << "Format of argument to VertexGen_Isotope::SetState: " << newline << " \"pname  specname  (Elo Ehi)\""
         << newline << " pname = particle name " << newline << " specname = Isotope name as given in ratdb " << newline
         << " Elo Ehi = optional limits on energy range of generated particles " << newline;
    return;
  }

  std::istringstream is(newValues.c_str());
  std::string pname, specname;
  // Read in the particle type
  is >> pname;
  if (is.fail() || pname.length() == 0) {
    Log::Die("VertexGen_Isotope: Incorrect vertex setting " + newValues);
  }
  G4ParticleDefinition *newTestGunG4Code = G4ParticleTable::GetParticleTable()->FindParticle(pname);
  if (newTestGunG4Code == NULL) {
    // not a particle name
    // see if we can parse it as an ion, e.g., U238 or Bi214
    std::string elementName;
    int A, Z;
    if (pname[1] >= '0' && pname[1] <= '9') {
      A = atoi(pname.substr(1).c_str());
      elementName = pname.substr(0, 1);
    } else {
      A = atoi(pname.substr(2).c_str());
      elementName = pname.substr(0, 2);
    }
    if (A > 0) {
      for (Z = 1; Z <= GLG4VertexGen_Gun::numberOfElements; Z++) {
        if (elementName == GLG4VertexGen_Gun::theElementNames[Z - 1]) break;
        if (Z <= GLG4VertexGen_Gun::numberOfElements) {
          newTestGunG4Code = G4IonTable::GetIonTable()->GetIon(Z, A, 0.0);
          info << " Isotope Vertex: Setting ion with A = " << A << " Z = " << Z << newline;
        }
      }
    }
    if (newTestGunG4Code == NULL) {
      warn << "Isotope Vertex: Could not find particle type " << pname << " defaulting to electron " << newline;
      _particle = "e-";
      return;
    }
  } else {
    info << "Isotope Vertex: Setting particle = " << pname << newline;
  }
  // so store the name and the particle definition
  _particle = pname;
  _pDef = newTestGunG4Code;

  // Read in the Isotope name
  is >> specname;
  // check that Isotope is in database - not sure what happens if this fails -
  // tidy!
  //        DBLinkPtr lspec = DB::Get()->GetLink("Isotope", specname);
  //        if(lspec){
  //            _Isotope = specname;
  //            info << "Isotope Vertex: Setting Isotope " << specname <<
  //            newline;
  //        }else{
  //            warn << "Could not find Isotope " << specname << " using
  //            default, flat Isotope " << newline;
  //        }

  // finally ready to initialise the Isotope!
  //        this->InitialiseIsotope();
  return;
}

///-------------------------------------------------------------------------
G4String VertexGen_Isotope::GetState() {
  // State setting is the particle name and Isotope name	- return it
  std::ostringstream os;
  os << _particle << " " << _Isotope << std::ends;
  G4String rv(os.str());
  return rv;
}

///-------------------------------------------------------------------------

void VertexGen_Isotope::SetIsotopeA(double IBDAm) {
  if ((IBDAm < 0.) || (IBDAm > 400.)) {
    warn << "Set your IBD Amplitude between 0 and 400." << newline;
    return;
  }
  valueA = IBDAm;
}

void VertexGen_Isotope::SetIsotopeZ(double IBDAm) {
  if ((IBDAm < 0.) || (IBDAm > 400.)) {
    warn << "Set your IBD Amplitude between 0 and 400." << newline;
    return;
  }
  valueZ = IBDAm;
}

void VertexGen_Isotope::SetIsotopeE(double IBDAm) {
  if ((IBDAm < 0.) || (IBDAm > 400.)) {
    warn << "Set your IBD Amplitude between 0 and 400." << newline;
    return;
  }
  valueE = IBDAm;
}

}  // namespace RAT
