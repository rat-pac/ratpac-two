// Uses the CRY generator (if installed) to generate cosmic-ray
// particle showers. CRY is available at http://nuclear.llnl.gov/simulation
#include <CRYGenerator.h>
#include <CRYSetup.h>

#include <RAT/VertexGen_CRY.hh>
// #include <RAT/CRYGenMessenger.hh>
#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>
#include <TTimeStamp.h>

#include <G4ParticleDefinition.hh>
#include <G4ParticleTable.hh>
#include <G4ThreeVector.hh>
#include <RAT/DB.hh>
#include <RAT/EventInfo.hh>
#include <RAT/Log.hh>
#include <Randomize.hh>
#include <string>

namespace RAT {
VertexGen_CRY::VertexGen_CRY(const char *arg_dbname) : GLG4VertexGen(arg_dbname) {
  // messenger
  DBLinkPtr crydb = DB::Get()->GetLink("CRY");
  // Types of secondary particles
  returnNeutrons = crydb->GetZ("returnNeutrons");
  returnProtons = crydb->GetZ("returnProtons");
  returnGammas = crydb->GetZ("returnGammas");
  returnElectrons = crydb->GetZ("returnElectrons");
  returnMuons = crydb->GetZ("returnMuons");
  returnPions = crydb->GetZ("returnPions");
  // Control / limit the types of interactions
  nParticlesMin = crydb->GetI("nParticlesMin");
  nParticlesMax = crydb->GetI("nParticlesMax");
  // Detector parameters
  altitude = crydb->GetD("altitude");  // Only allow 0, 2100, 11300
  latitude = crydb->GetD("latitude");  // -90 to 90
  date = crydb->GetS("date");
  // Lateral size of interest (meters)
  // Particles are return from a square plane of n x n meters, where n can
  // be 1, 3, 10, 30, 100, 300. Sizes between use the next size with truncation.
  subboxLength = crydb->GetI("subboxLength");
  // Build the relevant configuration components into a std::string to give to CRY
  std::string setupString("");
  setupString.append("returnNeutrons " + std::to_string(returnNeutrons) + " ");
  setupString.append("returnProtons " + std::to_string(returnProtons) + " ");
  setupString.append("returnGammas " + std::to_string(returnGammas) + " ");
  setupString.append("returnElectrons " + std::to_string(returnElectrons) + " ");
  setupString.append("returnMuons " + std::to_string(returnMuons) + " ");
  setupString.append("returnPions " + std::to_string(returnPions) + " ");
  setupString.append("nParticlesMin " + std::to_string(nParticlesMin) + " ");
  setupString.append("nParticlesMax " + std::to_string(nParticlesMax) + " ");
  setupString.append("altitude " + std::to_string(altitude) + " ");
  setupString.append("latitude " + std::to_string(latitude) + " ");
  setupString.append("date " + date + " ");
  setupString.append("subboxLength " + std::to_string(subboxLength) + " ");
  // Setup
  std::string crydirectory = static_cast<std::string>(getenv("CRYDATA"));
  CRYSetup *setup = new CRYSetup(setupString, crydirectory);
  setup->setRandomFunction([]() { return CLHEP::RandFlat::shoot(); });
  generator = new CRYGenerator(setup);
  // Setup the time based on the configuration date std::string
  int month = stoi(date.substr(0, date.find("-")));
  date.erase(0, date.find("-") + 1);
  int day = stoi(date.substr(0, date.find("-")));
  date.erase(0, date.find("-") + 1);
  int year = stoi(date.substr(0, date.find("-")));
  startTime.Set(year, month, day, 0, 0, 0, 0, 1, 0);
}
void VertexGen_CRY::GeneratePrimaryVertex(G4Event *event, G4ThreeVector &dx, G4double dt) {
  std::vector<CRYParticle *> *cryvector = new std::vector<CRYParticle *>;
  // Now the interesting bit
  generator->genEvent(cryvector);
  double timeSimulated = generator->timeSimulated();  // In seconds
  int seconds = static_cast<int>(floor(timeSimulated));
  int nanoseconds = static_cast<int>((timeSimulated - floor(timeSimulated)) * 1e9);
  TTimeStamp eventTime;
  eventTime.SetSec(startTime.GetSec() + seconds);
  eventTime.SetNanoSec(nanoseconds);
  // Now pass this time to geant-4
  EventInfo *exinfo = dynamic_cast<EventInfo *>(event->GetUserInformation());
  exinfo->utc = eventTime;
  // Convert the CRYParticle into a Geant-4 particle
  for (auto cryparticle : *cryvector) {
    std::string name = CRYUtils::partName(cryparticle->id());
    int PDGid = cryparticle->PDGid();
    double ke = cryparticle->ke();
    double x = cryparticle->x() * CLHEP::m;
    double y = cryparticle->y() * CLHEP::m;
    double z = cryparticle->z() * CLHEP::m;
    double u = cryparticle->u();
    double v = cryparticle->v();
    double w = cryparticle->w();
    double time = cryparticle->t();
    G4ThreeVector showerPosition(x, y, z);
    // construct a 4-momentum
    G4ParticleDefinition *pdef = G4ParticleTable::GetParticleTable()->FindParticle(PDGid);
    double mass = pdef->GetPDGMass();
    double norm = sqrt(u * u + v * v + w * w);
    double totalEnergy = ke + mass;
    double px = sqrt(totalEnergy * totalEnergy - mass * mass) * u / norm;
    double py = sqrt(totalEnergy * totalEnergy - mass * mass) * v / norm;
    double pz = sqrt(totalEnergy * totalEnergy - mass * mass) * w / norm;
    G4ThreeVector position = dx + showerPosition;
    // Geant-4 Particle
    G4PrimaryVertex *vertex = new G4PrimaryVertex(position, time);
    G4PrimaryParticle *g4particle = new G4PrimaryParticle(pdef, px, py, pz, totalEnergy);
    vertex->SetPrimary(g4particle);
    event->AddPrimaryVertex(vertex);
  }
}

void VertexGen_CRY::SetState(G4String newValues) {
  // newValues = util_strip_default(newValues);
  if (newValues.length() == 0) {
    info << "Current state of this VertexGen_CRY: " << GetState() << newline;
  }
}

G4String VertexGen_CRY::GetState() {
  std::string returnvalue("");
  return returnvalue;
}

}  // namespace RAT
