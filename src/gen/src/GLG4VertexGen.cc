/** @file
   Source code for GenericLAND global vertex generators for primary events,
   (See note on GenericLAND generators for more information.)

   This file is part of the GenericLAND software library.
   $Id: GLG4VertexGen.cc,v 1.2 2005/11/19 17:59:39 volsung Exp $

   @author G.Horton-Smith, August 3, 2001
*/

#include "RAT/GLG4VertexGen.hh"

#include <RAT/Log.hh>
#include <sstream>
#include <string>

#include "CLHEP/Units/PhysicalConstants.h"
#include "G4Event.hh"
#include "G4HEPEvtParticle.hh"
#include "G4IonTable.hh"
#include "G4Ions.hh"
#include "G4OpticalPhoton.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"
#include "G4RunManager.hh"
#include "G4Track.hh"
#include "RAT/GLG4StringUtil.hh"
#include "Randomize.hh"
#include "globals.hh"

#if defined(__GNUC__) && __GNUC__ < 3
extern "C" {
FILE *popen(const char *__command, const char *__modes) throw();
int pclose(FILE *__stream) throw();
}
#else
#include <stdio.h>  // for popen
#endif

// we assume STL compatibility below
#include <vector>

////////////////////////////////////////////////////////////////
const char *GLG4VertexGen_Gun::theElementNames[] = {
    "H",  "He", "Li", "Be", "B",  "C",  "N",  "O",  "F",  "Ne", "Na", "Mg", "Al", "Si", "P",  "S",  "Cl", "Ar", "K",
    "Ca", "Sc", "Ti", "V",  "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn", "Ga", "Ge", "As", "Se", "Br", "Kr", "Rb", "Sr",
    "Y",  "Zr", "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag", "Cd", "In", "Sn", "Sb", "Te", "I",  "Xe", "Cs", "Ba", "La",
    "Ce", "Pr", "Nd", "Pm", "Sm", "Eu", "Gd", "Tb", "Dy", "Ho", "Er", "Tm", "Yb", "Lu", "Hf", "Ta", "W",  "Re", "Os",
    "Ir", "Pt", "Au", "Hg", "Tl", "Pb", "Bi", "Po", "At", "Rn", "Fr", "Ra", "Ac", "Th", "Pa", "U",  "Np", "Pu", "Am",
    "Cm", "Bk", "Cf", "Es", "Fm", "Md", "No", "Lr", "Rf", "Db", "Sg", "Bh", "Hs", "Mt", "Xa"

};

GLG4VertexGen_Gun::GLG4VertexGen_Gun(const char *arg_dbname)
    : GLG4VertexGen(arg_dbname), _mom(0., 0., 0.), _ke(0.0), _pol(0., 0., 0.), _multiplicity(1) {
  _pDef = G4ParticleTable::GetParticleTable()->FindParticle("geantino");
}

GLG4VertexGen_Gun::~GLG4VertexGen_Gun() {}

/** Generates one or more particles with a specified momentum, or isotropic
    momentum with specific energy, and specified polarization, or uniformly
    random polarization, based on parameters set via SetState() */
void GLG4VertexGen_Gun::GeneratePrimaryVertex(G4Event *argEvent, G4ThreeVector &dx, G4double dt) {
  for (int imult = 0; imult < _multiplicity; imult++) {
    G4PrimaryVertex *vertex = new G4PrimaryVertex(dx, dt);

    if (_pDef == 0) {
      _pDef = G4ParticleTable::GetParticleTable()->FindParticle("geantino");
    }
    G4double mass = _pDef->GetPDGMass();

    G4PrimaryParticle *particle;

    G4ThreeVector dir;
    if (_mom.mag2() > 0.0) {
      // fixed momentum and direction
      particle = new G4PrimaryParticle(_pDef,      // particle code
                                       _mom.x(),   // x component of momentum
                                       _mom.y(),   // y component of momentum
                                       _mom.z());  // z component of momentum
      dir = _mom.unit();
    } else {
      // isotropic direction
      /* generate random ThreeVector of unit length isotropically
         distributed on sphere by cutting two circles from a rectangular
         area and std::mapping circles onto top and bottom of sphere.
         N.B. the square of the radius is a uniform random variable.
         -GAHS.
      */
      while (1) {
        double u, v, q2;
        // try first circle
        u = 3.5 * G4UniformRand() - 1.75;
        v = 2.0 * G4UniformRand() - 1.0;
        q2 = u * u + v * v;
        if (q2 < 1.0) {
          double rho_over_q = sqrt(2.0 - q2);
          dir = G4ThreeVector(u * rho_over_q, v * rho_over_q, 1.0 - q2);
          break;
        }
        // try second circle
        u = (u > 0.0) ? u - 1.75 : u + 1.75;
        v = (v > 0.0) ? v - 1.00 : v + 1.00;
        q2 = u * u + v * v;
        if (q2 < 1.0) {
          double rho_over_q = sqrt(2.0 - q2);
          dir = G4ThreeVector(u * rho_over_q, v * rho_over_q, q2 - 1.0);
          break;
        }
      }
      G4ThreeVector rmom(dir * sqrt(_ke * (_ke + 2. * mass)));
      particle = new G4PrimaryParticle(_pDef,      // particle code
                                       rmom.x(),   // x component of momentum
                                       rmom.y(),   // y component of momentum
                                       rmom.z());  // z component of momentum
    }

    // adjust polarization if needed
    if (_pDef->GetPDGSpin() == 1.0 && mass == 0.0) {
      // spin 1 mass 0 particles should have transverse polarization
      G4ThreeVector rpol = _pol - dir * (dir * _pol);
      G4double rpolmag = rpol.mag();
      if (rpolmag > 0.0) {
        // use projected orthogonal pol, normalized
        rpol *= (1.0 / rpolmag);
      } else {
        // choose random pol
        G4double phi = (G4UniformRand() * 2.0 - 1.0) * M_PI;
        G4ThreeVector e1 = dir.orthogonal().unit();
        G4ThreeVector e2 = dir.cross(e1);
        rpol = e1 * cos(phi) + e2 * sin(phi);
      }
      particle->SetPolarization(rpol.x(), rpol.y(), rpol.z());
    } else {
      // use user-set polarization unmodified
      particle->SetPolarization(_pol.x(), _pol.y(), _pol.z());
    }

    particle->SetMass(mass);  // Geant4 is silly.

    vertex->SetPrimary(particle);

    argEvent->AddPrimaryVertex(vertex);
  }
}

/** Set state of the GLG4VertexGen_Gun, or show current state and
    explanation of state syntax if empty string provided.

    Format of argument to GLG4VertexGen_Gun::SetState:
       "pname  momx_MeV momy_MeV momz_MeV  KE_MeV  polx poly polz mult",
     where
      - pname is the name of a particle type (e-, mu-, U238, ...)
      - mom*_MeV is a momentum in MeV/c
      - KE_MeV is an optional override for kinetic energy
      - pol* is an optional polarization std::vector.
      - mult is an optional multiplicity of the particles.
     If mom*_MeV==0 and KE_MeV!=0, then random isotropic directions chosen.
     If pol*==0, then random transverse polarization for photons.
*/
void GLG4VertexGen_Gun::SetState(G4String newValues) {
  newValues = util_strip_default(newValues);
  if (newValues.length() == 0) {
    // print help and current state
    RAT::info << "Current state of this GLG4VertexGen_Gun:" << newline << " \"" << GetState() << "\"" << newline
              << newline;
    RAT::info << "Format of argument to GLG4VertexGen_Gun::SetState: " << newline
              << " \"pname  momx_MeV momy_MeV momz_MeV  KE_MeV  polx poly polz "
              << "mult\"\n"
              << " where pname is the name of a particle type (e-, mu-, U238, ...)\n"
              << " mom*_MeV is a momentum in MeV/c\n"
              << " KE_MeV is an optional override for kinetic energy\n"
              << " pol* is an optional polarization std::vector.\n"
              << " mult is an optional multiplicity of the particles.\n"
              << "mom*_MeV==0 and KE_MeV!=0 --> random isotropic directions chosen\n"
              << "pol*==0 --> random transverse polarization for photons.\n"
              << newline;
    return;
  }

  std::istringstream is(newValues.c_str());

  // set particle
  std::string pname;
  is >> pname;
  if (is.fail() || pname.length() == 0) return;
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
      for (Z = 1; Z <= numberOfElements; Z++)
        if (elementName == theElementNames[Z - 1]) break;
      if (Z <= numberOfElements) newTestGunG4Code = G4IonTable::GetIonTable()->GetIon(Z, A, 0.0);
    }
    if (newTestGunG4Code == NULL) {
      RAT::warn << "test gun particle type not changed! Could not"
                   " find "
                << pname << newline;
      return;
    }
  }
  _pDef = newTestGunG4Code;

  // set momentum
  G4double x, y, z;
  is >> x >> y >> z;
  if (is.fail()) return;
  _mom = G4ThreeVector(x * CLHEP::MeV, y * CLHEP::MeV, z * CLHEP::MeV);
  _ke = sqrt(_mom.mag2() + _pDef->GetPDGMass() * _pDef->GetPDGMass()) - _pDef->GetPDGMass();

  // optional override of kinetic energy
  G4double p_renorm;
  is >> x;
  if (is.fail()) {
    return;
  }
  if (x <= 0.0) {
    p_renorm = 0.0;
    _ke = 0.0;
  } else {
    _ke = x * CLHEP::MeV;
    if (_mom.mag2() <= 0.0)
      p_renorm = 0.0;
    else
      p_renorm = sqrt(_ke * (_ke + 2.0 * newTestGunG4Code->GetPDGMass()) / _mom.mag2());
  }
  _mom *= p_renorm;

  // set particle polarization
  is >> x >> y >> z;
  if (is.fail()) return;
  _pol = G4ThreeVector(x, y, z);

  // set multiplicity
  is >> _multiplicity;
}

G4String GLG4VertexGen_Gun::GetState() {
  std::ostringstream os;

  if (_pDef == 0) {
    _pDef = G4ParticleTable::GetParticleTable()->FindParticle("geantino");
  }
  os << _pDef->GetParticleName() << '\t' << _mom.x() << ' ' << _mom.y() << ' ' << _mom.z() << '\t' << _ke << '\t'
     << _pol.x() << ' ' << _pol.y() << ' ' << _pol.z() << '\t' << _multiplicity << std::ends;
  G4String rv(os.str());
  return rv;
}

////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////   Gun V2
const char *GLG4VertexGen_Gun2::theElementNames[] = {
    "H",  "He", "Li", "Be", "B",  "C",  "N",  "O",  "F",  "Ne", "Na", "Mg", "Al", "Si", "P",  "S",  "Cl", "Ar", "K",
    "Ca", "Sc", "Ti", "V",  "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn", "Ga", "Ge", "As", "Se", "Br", "Kr", "Rb", "Sr",
    "Y",  "Zr", "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag", "Cd", "In", "Sn", "Sb", "Te", "I",  "Xe", "Cs", "Ba", "La",
    "Ce", "Pr", "Nd", "Pm", "Sm", "Eu", "Gd", "Tb", "Dy", "Ho", "Er", "Tm", "Yb", "Lu", "Hf", "Ta", "W",  "Re", "Os",
    "Ir", "Pt", "Au", "Hg", "Tl", "Pb", "Bi", "Po", "At", "Rn", "Fr", "Ra", "Ac", "Th", "Pa", "U",  "Np", "Pu", "Am",
    "Cm", "Bk", "Cf", "Es", "Fm", "Md", "No", "Lr", "Rf", "Db", "Sg", "Bh", "Hs", "Mt", "Xa"

};

GLG4VertexGen_Gun2::GLG4VertexGen_Gun2(const char *arg_dbname)
    : GLG4VertexGen(arg_dbname), _mom(0., 0., 0.), _ke1(0.0), _ke2(0.0), _pol(0., 0., 0.), _multiplicity(1) {
  _pDef = G4ParticleTable::GetParticleTable()->FindParticle("geantino");
}

GLG4VertexGen_Gun2::~GLG4VertexGen_Gun2() {}

/** Generates one or more particles with a specified momentum, or isotropic
    momentum with specific energy, and specified polarization, or uniformly
    random polarization, based on parameters set via SetState() */
void GLG4VertexGen_Gun2::GeneratePrimaryVertex(G4Event *argEvent, G4ThreeVector &dx, G4double dt) {
  for (int imult = 0; imult < _multiplicity; imult++) {
    G4PrimaryVertex *vertex = new G4PrimaryVertex(dx, dt);

    if (_pDef == 0) {
      _pDef = G4ParticleTable::GetParticleTable()->FindParticle("geantino");
    }
    G4double mass = _pDef->GetPDGMass();

    G4PrimaryParticle *particle;

    G4double temp_ke_max;
    G4double temp_ke_min;

    if (_ke1 > _ke2) {
      temp_ke_max = _ke1;
      temp_ke_min = _ke2;
      _ke1 = temp_ke_min;
      _ke2 = temp_ke_max;
    }

    G4double _ke = (_ke2 - _ke1) * G4UniformRand() + _ke1;

    G4ThreeVector dir;
    if (_mom.mag2() > 0.0) {
      if (_mom_theta > 0.0)  // emission into a cone of angle _mom_theta
      {
        G4double phi = 2. * CLHEP::pi * G4UniformRand();
        G4double cosTheta = 1.0 - (1.0 - cos(_mom_theta)) * G4UniformRand();
        G4double sinTheta = sqrt(1. - cosTheta * cosTheta);

        G4double ux = sinTheta * cos(phi);
        G4double uy = sinTheta * sin(phi);
        G4double uz = cosTheta;

        dir = G4ThreeVector(ux, uy, uz);

        dir.rotateY(_mom.theta());
        dir.rotateZ(_mom.phi());
      } else
        dir = _mom;

      dir *= sqrt(_ke * (_ke + 2 * mass) / dir.mag2());
      particle = new G4PrimaryParticle(_pDef,     // particle code
                                       dir.x(),   // x component of momentum
                                       dir.y(),   // y component of momentum
                                       dir.z());  // z component of momentum
      dir = dir.unit();

    } else {
      // isotropic direction

      G4double phi = 2. * CLHEP::pi * G4UniformRand();
      G4double cosTheta = -1. + 2. * G4UniformRand();
      G4double sinTheta = sqrt(1. - cosTheta * cosTheta);

      G4double ux = sinTheta * cos(phi);
      G4double uy = sinTheta * sin(phi);
      G4double uz = cosTheta;

      dir = G4ThreeVector(ux, uy, uz);

      G4ThreeVector rmom(dir * sqrt(_ke * (_ke + 2. * mass)));
      particle = new G4PrimaryParticle(_pDef,      // particle code
                                       rmom.x(),   // x component of momentum
                                       rmom.y(),   // y component of momentum
                                       rmom.z());  // z component of momentum
    }

    // adjust polarization if needed
    if (_pDef->GetPDGSpin() == 1.0 && mass == 0.0) {
      // spin 1 mass 0 particles should have transverse polarization
      G4ThreeVector rpol = _pol - dir * (dir * _pol);
      G4double rpolmag = rpol.mag();
      if (rpolmag > 0.0) {
        // use projected orthogonal pol, normalized
        rpol *= (1.0 / rpolmag);
      } else {
        // choose random pol
        G4double phi = (G4UniformRand() * 2.0 - 1.0) * M_PI;
        G4ThreeVector e1 = dir.orthogonal().unit();
        G4ThreeVector e2 = dir.cross(e1);
        rpol = e1 * cos(phi) + e2 * sin(phi);
      }
      particle->SetPolarization(rpol.x(), rpol.y(), rpol.z());
    } else {
      // use user-set polarization unmodified
      particle->SetPolarization(_pol.x(), _pol.y(), _pol.z());
    }

    particle->SetMass(mass);  // Geant4 is silly.

    vertex->SetPrimary(particle);

    argEvent->AddPrimaryVertex(vertex);
  }
}

/** Set state of the GLG4VertexGen_Gun, or show current state and
    explanation of state syntax if empty string provided.

    Format of argument to GLG4VertexGen_Gun::SetState:
       "pname  momx_MeV momy_MeV momz_MeV  KE_MeV  polx poly polz mult",
     where
      - pname is the name of a particle type (e-, mu-, U238, ...)
      - mom*_MeV is a momentum in MeV/c
      - KE_MeV is an optional override for kinetic energy
      - pol* is an optional polarization std::vector.
      - mult is an optional multiplicity of the particles.
     If mom*_MeV==0 and KE_MeV!=0, then random isotropic directions chosen.
     If pol*==0, then random transverse polarization for photons.
*/
void GLG4VertexGen_Gun2::SetState(G4String newValues) {
  newValues = util_strip_default(newValues);
  if (newValues.length() == 0) {
    // print help and current state
    RAT::info << "Current state of this GLG4VertexGen_Gun:" << newline << " \"" << GetState() << "\"" << newline
              << newline;
    RAT::info << "Format of argument to GLG4VertexGen_Gun::SetState: " << newline
              << " \"pname  momx_MeV momy_MeV momz_MeV angle KE1_MeV  KE2_MeV polx "
              << "ply polz multiplicity\"\n"
              << " where pname is the name of a particle type (e-, mu-, U238, ...)\n"
              << " mom*_MeV is a momentum in MeV/c\n"
              << " angle is the aperture of the shooting cone\n"
              << " KE1_MeV is the minimum kinetic energy\n"
              << " KE2_MeV is the maximum kinetic energy\n"
              << " pol* is a polarization\n"
              << " multiplicity = (optional) no. of primaries shot at once\n"
              << " mom*_MeV==0  --> random isotropic directions chosen\n"
              << " pol*==0  --> random isotropic polarizations\n"
              << newline;
    return;
  }

  std::istringstream is(newValues.c_str());

  // set particle
  std::string pname;
  is >> pname;
  if (is.fail() || pname.length() == 0) return;
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
      for (Z = 1; Z <= numberOfElements; Z++)
        if (elementName == theElementNames[Z - 1]) break;
      if (Z <= numberOfElements) newTestGunG4Code = G4IonTable::GetIonTable()->GetIon(Z, A, 0.0);
    }
    if (newTestGunG4Code == NULL) {
      RAT::warn << "test gun particle type not changed! Could not"
                   " find "
                << pname << newline;
      return;
    }
  }
  _pDef = newTestGunG4Code;

  // set momentum
  G4double x, y, z;
  is >> x >> y >> z;
  if (is.fail()) return;
  _mom = G4ThreeVector(x, y, z);

  is >> x;
  if (is.fail()) return;
  _mom_theta = x * CLHEP::deg;

  is >> x >> y;
  if (is.fail()) return;
  _ke1 = x * CLHEP::MeV;
  _ke2 = y * CLHEP::MeV;

  // set particle polarization
  is >> x >> y >> z;
  if (is.fail()) return;
  _pol = G4ThreeVector(x, y, z);

  // set multiplicity
  is >> _multiplicity;
}

G4String GLG4VertexGen_Gun2::GetState() {
  std::ostringstream os;

  if (_pDef == 0) {
    _pDef = G4ParticleTable::GetParticleTable()->FindParticle("geantino");
  }
  os << _pDef->GetParticleName() << '\t' << _mom.x() << ' ' << _mom.y() << ' ' << _mom.z() << '\t' << _ke1 << '\t'
     << _ke2 << '\t' << std::ends;
  G4String rv(os.str());
  return rv;
}

////////////////////////////////////////////////////////////////  end Gun2

GLG4VertexGen_HEPEvt::GLG4VertexGen_HEPEvt(const char *arg_dbname) : GLG4VertexGen(arg_dbname) {
  _filename = "";
  _file = 0;
  _isPipe = false;
  _useExternalPos = false;
}

GLG4VertexGen_HEPEvt::~GLG4VertexGen_HEPEvt() { Close(); }

void GLG4VertexGen_HEPEvt::Open(const char *argFilename) {
  if (argFilename == NULL || argFilename[0] == '\0') {
    RAT::warn << "GLG4VertexGen_HEPEvt::Open(): null filename" << newline;
    return;
  }
  if (_file != 0) Close();

  _filename = argFilename;

  // is a pipe requested?
  if (argFilename[strlen(argFilename) - 1] == '|') {
    _isPipe = true;
    _file = popen(_filename.substr(0, _filename.length() - 1).c_str(), "r");
    if (!_file) {
      perror(_filename.c_str());
      _filename = "";
      _isPipe = false;
      return;
    }
  } else {
    _isPipe = false;
    _file = fopen(argFilename, "r");
    if (!_file) {
      perror(_filename.c_str());
      _filename = "";
      return;
    }
  }
}

void GLG4VertexGen_HEPEvt::Close() {
  if (_file) {
    if (_isPipe) {
      int status = pclose(_file);
      if (status != 0) {
        RAT::warn << "HEPEvt input pipe from " << _filename << " gave status " << status << " on close." << newline;
      }
      _file = 0;
      _isPipe = false;
      _filename = "";
    } else {
      fclose(_file);
      _file = 0;
      _filename = "";
    }
  }
}

void GLG4VertexGen_HEPEvt::GetDataLine(char *buffer, size_t size) {
  int rewind_count = 0;
  char firstword[64];

  for (;;) {
    buffer[0] = '\0';
    if (fgets(buffer, size, _file) != buffer) {
      // try from beginning of file on failure
      RAT::warn << (feof(_file) ? "End-of_file reached" : "Failure") << " on " << _filename << " at " << ftell(_file);
      RAT::warn << " Executive decision to end at end of file, soft closing." << newline;
      G4RunManager::GetRunManager()->AbortRun(true);
      if (rewind_count == 0) {
        if (_isPipe == false) {
          RAT::warn << ", rewinding." << newline;
          rewind(_file);
        } else {
          RAT::warn << ", reopening." << newline;
          pclose(_file);
          _file = popen(_filename.substr(0, _filename.length() - 1).c_str(), "r");
        }
        rewind_count++;
        continue;
      } else {
        RAT::warn << ", closing." << newline;
        Close();
        return;
      }
    }
    // check for "sentinel" block

    firstword[0] = '\0';
    sscanf(buffer, " %63s", firstword);                         // leading space means skip whitespace
    if (firstword[0] == '#' || firstword[0] == '\0') continue;  // This is a comment line or blank line
    if (firstword[0] >= '0' && firstword[0] <= '9') break;  // found numeric data -- first value is always non-neg int!
    if (strcmp(firstword, "STATE") == 0) {
      double value;
      int nscan;
      while (fgets(buffer, size, _file) == buffer) {
        firstword[0] = '\0';
        nscan = sscanf(buffer, " %63s %lf", firstword, &value);
        if (firstword[0] == '#' || firstword[0] == '\0') continue;
        if (strcmp(firstword, "ENDSTATE") == 0) break;
        if (nscan == 2)
          RAT::Log::Die(
              "This code no longer works due to removal of GLG4param.  Check "
              "revision history");  // db[ ((_dbname+".")+firstword).c_str() ]=
                                    // value;
        else {
          RAT::info << "Warning, bad STATE line in " << _filename << ": " << buffer << newline;
        }
      }
      continue;
    } else if (strcmp(firstword, "HEPEVT") == 0) {
      char sentinel[20];
      sentinel[0] = '\0';
      sscanf(buffer, " HEPEVT DATA SENTINEL IS %19s", sentinel);
      if (sentinel[0] == '\0') {
        RAT::warn << "Warning, line starting with word HEPEVT ignored" << newline;
        continue;
      }
      // skip until sentinel found
      while (fgets(buffer, size, _file) == buffer) {
        firstword[0] = '\0';
        sscanf(buffer, " %63s", firstword);
        if (strcmp(firstword, sentinel) == 0) break;
      }
      continue;
    }

    // not comment, blank line, STATE, or numeric data
    RAT::warn << "Warning, ignoring garbage line in " << _filename << newline << buffer;
  }
  // Peak at next word
  long posnot = ftell(_file);
  char buff2[400];
  if (fgets(buff2, sizeof(buff2), _file) != buff2) {
    RAT::warn << " Executive decision to end at end of file, soft closing." << newline;
    G4RunManager::GetRunManager()->AbortRun(true);
  }
  fseek(_file, posnot, SEEK_SET);
}

/** Generates one or more particles with type, momentum, and
    optional time offset, spatial offset, and polarization,
    based on lines read from file via SetState().

    The file has the standard HEPEVT foramt. Below is the documentation for this format,
    described in the [MARLEY documentation](https://www.marleygen.org/interpret_output.html#hepevt):
    ```
    NEVHEP NHEP
    ```
    where NEVHEP is the event number and NHEP is the number of particles in the event.
    The header is followed by NHEP lines, each representing a single particle. These have the
    format:

    ```
    ISTHEP IDHEP JMOHEP1 JMOHEP2 JDAHEP1 JDAHEP2 PHEP1 PHEP2 PHEP3 PHEP4 PHEP5 VHEP1 VHEP2 VHEP3 VHEP4
    ```

    where ISTHEP is an integer code identifying the particle status and IDHEP is the particle’s PDG code. In agreement
    with the HEPEVT standard, MARLEY uses status code 1 for the final-state particles and 3 for the initial-state
    particles. The JMOHEP1, JMOHEP2, JDAHEP1, and JDAHEP2 entries record the indices (between 1 and NHEP, inclusive) of
    particles in the event record that correspond to the first mother, second mother, first daughter, and last daughter
    of the current particle, respectively. These indices are set to zero in cases where they do not apply (e.g., a
    particle with no daughters will have JDAHEP1 = JDAHEP2 = 0). Entries PHEP1 through PHEP3 record the x-, y-, and
    z-components of the particle 3-momentum, while PHEP4 gives the total energy and PHEP5 gives the particle mass (all
   in GeV). Entries VHEP1 through VHEP3 store the x, y, and z positions of the particle production vertex (mm), and
   VHEP4 gives the production time (mm/c). Additional References: https://doi.org/10.5170/CERN-1989-008-V-3
*/
void GLG4VertexGen_HEPEvt::GeneratePrimaryVertex(G4Event *argEvent, G4ThreeVector &dx, G4double dt) {
  // this is a modified and adapted version of G4HEPEvt
  // (which itself may be a modified and adapted version of CLHEP/StdHep++...)

  if (_file == 0) {
    RAT::warn << "GLG4VertexGen_HEPEvt::GeneratePrimaryVertex: "
                 "Error, no file open!"
              << newline;
    return;
  }

  char buffer[400];
  int istat;
  int NEVHEP;  // event number, not used.
  int NHEP;    // number of entries
  GetDataLine(buffer, sizeof(buffer));
  if (_file == 0) {
    RAT::warn << "Unexpected end of file in " << _filename << ", expecting NHEP." << newline;
    Close();
    return;
  }
  istat = sscanf(buffer, "%d %d", &NEVHEP, &NHEP);
  if (istat != 2) {
    // this should never happen: GetDataLine() should make sure integer is ok
    // -- but the test above is cheap and a good cross-check, so leave it.
    RAT::warn << "Bad data in " << _filename << ", expecting NHEP but got:" << newline << buffer << " --> closing file."
              << newline;
    Close();
    return;
  }

  std::vector<G4HEPEvtParticle *> HPlist;
  std::vector<G4PrimaryVertex *> vertexList;  // same indices as HPList
  std::vector<G4PrimaryVertex *> vertexSet;   // bag of unique vertices
  vertexSet.push_back(new G4PrimaryVertex(dx, dt));

  G4double vertexX = 0.0, vertexY = 0.0, vertexZ = 0.0, vertexT = 0.0;

  for (int IHEP = 0; IHEP < NHEP; IHEP++) {
    G4int ISTHEP = 0;                    // status code
    G4int IDHEP = 0;                     // HEP PDG code
    G4int JMOHEP1 = 0;                   // first mother
    G4int JMOHEP2 = 0;                   // last mother
    G4int JDAHEP1 = 0;                   // first daughter
    G4int JDAHEP2 = 0;                   // last daughter
    G4double PHEP1 = 0.0;                // px in GeV
    G4double PHEP2 = 0.0;                // py in GeV
    G4double PHEP3 = 0.0;                // pz in GeV
    G4double PHEP4 = 0.0;                // total energy in GeV
    G4double req_novalue = 1e30;         // larger than the diameter of univers in mm
    G4double PHEP5 = req_novalue;        // mass in GeV
    G4double req_vertexX = req_novalue;  // x vertex in mm requested on this line
    G4double req_vertexY = req_novalue;  // y vertex in mm "
    G4double req_vertexZ = req_novalue;  // z vertex in mm "
    G4double req_vertexT = req_novalue;  // vertex time (in ns, a GLG4HEPEvt convention)
    G4double polx = 0.0;                 // x polarization
    G4double poly = 0.0;                 // y polarization
    G4double polz = 0.0;                 // z polarization
    G4double energy_unit = CLHEP::GeV;
    G4double position_unit = CLHEP::mm;
    G4double time_unit = CLHEP::mm / CLHEP::c_light;

    GetDataLine(buffer, sizeof(buffer));
    if (_file == 0) {
      RAT::warn << "Unexpected end of file in " << _filename << ", expecting particle " << IHEP << "/" << NHEP
                << newline;
      Close();
      return;
    }
    std::istringstream is(buffer);
    // if request was made to use positions listed in ascii input,
    // read them into req_vertexX, Y, and Z
    if (_useExternalPos) {
      is >> ISTHEP >> IDHEP >> JMOHEP1 >> JMOHEP2 >> JDAHEP1 >> JDAHEP2 >> PHEP1 >> PHEP2 >> PHEP3 >> PHEP4 >> PHEP5 >>
          req_vertexX >> req_vertexY >> req_vertexZ >> req_vertexT;
      // print an error and Close() if request was made to use external
      // positions, but they are not present in ascii input
      if (req_vertexX == req_novalue && req_vertexY == req_novalue && req_vertexZ == req_novalue) {
        RAT::warn << "GLG4VertexGen: Request was made to use external position "
                     "for event,\n"
                  << " but no positions were found in this line of ascii input." << newline
                  << " No events will be generated for this line." << newline;
        return;
      }
    } else {  // use positions from a different position generator,
              // not the content of the ascii input
      is >> ISTHEP >> IDHEP >> JMOHEP1 >> JMOHEP2 >> JDAHEP1 >> JDAHEP2 >> PHEP1 >> PHEP2 >> PHEP3 >> PHEP4 >> PHEP5;
    }

    // reset units if "dimensionless" pseudo-particle information
    if (ISTHEP >= kISTHEP_InformatonMin) energy_unit = position_unit = time_unit = 1.0;

    // NOTE: previously doing some crazy stuff, frobnicating IDHEP if ISTHEP != 1
    if (ISTHEP != kISTHEP_ParticleForTracking) {
      continue;
    }

    // create G4PrimaryParticle object
    // create an "ion" (nucleus) if IDHEP>9800000
    G4PrimaryParticle *particle;
    if (IDHEP > kIonCodeOffset && IDHEP < kIonCodeOffset + 99999) {
      int A = IDHEP % 1000;
      int Z = (IDHEP / 1000) % 100;
      G4ParticleDefinition *g4code = G4IonTable::GetIonTable()->GetIon(Z, A, 0.0);
      if (g4code == 0) {
        RAT::warn << "Warning: GLG4HEPEvt could not find Ion Z=" << Z << " A=" << A << " code=" << IDHEP << newline;
        particle = new G4PrimaryParticle(IDHEP, PHEP1 * energy_unit, PHEP2 * energy_unit, PHEP3 * energy_unit);
      } else {
        particle = new G4PrimaryParticle(g4code, PHEP1 * energy_unit, PHEP2 * energy_unit, PHEP3 * energy_unit);
      }
    } else if (IDHEP == 0 && fabs(PHEP1) + fabs(PHEP2) + fabs(PHEP3) < 20e-9) {
      // special case: E less than ~10 eV and pdgcode=0 means optical photon
      particle = new G4PrimaryParticle(G4OpticalPhoton::OpticalPhoton(), PHEP1 * energy_unit, PHEP2 * energy_unit,
                                       PHEP3 * energy_unit);
    } else {
      particle = new G4PrimaryParticle(IDHEP, PHEP1 * energy_unit, PHEP2 * energy_unit, PHEP3 * energy_unit);
    }
    if (polx != 0.0 || poly != 0.0 || polz != 0.0) particle->SetPolarization(polx, poly, polz);

    particle->SetMass(PHEP5 * energy_unit);

    // create G4HEPEvtParticle object
    G4HEPEvtParticle *hepParticle = new G4HEPEvtParticle(particle, ISTHEP, JDAHEP1, JDAHEP2);

    // Store
    HPlist.push_back(hepParticle);

    // find or make the right vertex for this time and position
    // as a special case, if a line doesn't have a value for position or time,
    // then use the position and time previously set
    if (_useExternalPos) {
      // update vertex positions, and see if we can reuse an existing vertex
      if (req_vertexX != req_novalue) vertexX = req_vertexX * position_unit;
      if (req_vertexY != req_novalue) vertexY = req_vertexY * position_unit;
      if (req_vertexZ != req_novalue) vertexZ = req_vertexZ * position_unit;
      if (req_vertexT != req_novalue) vertexT = req_vertexT * time_unit;
      G4ThreeVector position(vertexX, vertexY, vertexZ);
      int iv;
      for (iv = vertexSet.size() - 1; iv >= 0; iv--) {
        if (vertexSet[iv]->GetT0() == vertexT && vertexSet[iv]->GetPosition() == position) break;  // found it
      }
      if (iv >= 0)  // found?
        vertexList.push_back(vertexSet[iv]);
      else {  // not found.
        G4PrimaryVertex *nv = new G4PrimaryVertex(position, vertexT);
        vertexSet.push_back(nv);
        vertexList.push_back(nv);
      }
    } else {
      // no change, use current vertex
      vertexList.push_back(vertexSet[vertexSet.size() - 1]);
    }
  }

  // check if there is at least one particle
  if (HPlist.empty()) return;

  // make connection between daughter particles decayed from
  // the same mother
  for (size_t i = 0; i < HPlist.size(); i++) {
    if (HPlist[i]->GetJDAHEP1() > 0)  //  it has daughters
    {
      int jda1 = HPlist[i]->GetJDAHEP1() - 1;  // FORTRAN index starts from 1
      int jda2 = HPlist[i]->GetJDAHEP2() - 1;  // but C++ starts from 0.
      G4PrimaryParticle *mother = HPlist[i]->GetTheParticle();
      for (int j = jda1; j <= jda2; j++) {
        G4PrimaryParticle *daughter = HPlist[j]->GetTheParticle();
        if (HPlist[j]->GetISTHEP() > 0) {
          if (vertexList[i] == vertexList[j]) {
            mother->SetDaughter(daughter);
            HPlist[j]->Done();
          } else {
            RAT::warn << "Error in  GLG4VertexGen_HEPEvt[" << _filename << "]: mother " << i << " and daughter " << j
                      << " are at different vertices, and this cannot be"
                      << " done in Geant4! Must divorce mother and daughter." << newline;
          }
        }
      }
    }
  }

  // put initial particles to the vertex (or vertices)
  // NOTE: Is this actually true? Don't we want to track the daughters?
  for (size_t ii = 0; ii < HPlist.size(); ii++) {
    if (HPlist[ii]->GetISTHEP() > 0)  // ISTHEP of daughters had been
                                      // set to negative
    {
      G4PrimaryParticle *initialParticle = HPlist[ii]->GetTheParticle();
      vertexList[ii]->SetPrimary(initialParticle);
    }
  }

  // clear G4HEPEvtParticles
  for (size_t iii = 0; iii < HPlist.size(); iii++) {
    delete HPlist[iii];
  }
  HPlist.clear();

  // Put the vertex (or vertices) to G4Event object
  {
    for (size_t iv = 0; iv < vertexSet.size(); iv++) {
      argEvent->AddPrimaryVertex(vertexSet[iv]);
    }
  }
}

/** Set source of HEPEVT ascii data for this generator, or print
    current source and help.
    Format of string: either "filename" or "shell_command (arguments) |".
*/
void GLG4VertexGen_HEPEvt::SetState(G4String newValues) {
  newValues = util_strip_default(newValues);
  if (newValues.length() == 0) {
    // print help and current state
    RAT::info << "Current state of this GLG4VertexGen_HEPEvt:" << newline << " \"" << GetState() << "\"" << newline
              << newline;
    RAT::info << "Format of argument to GLG4VertexGen_HEPEvt::SetState: " << newline
              << "either \"filename\" or \"shell_command (arguments) |\"" << newline << newline;
    return;
  }

  Open(newValues);
}

G4String GLG4VertexGen_HEPEvt::GetState() { return G4String(_filename); }

////////////////////////////////////////////////////////////////

#if 0
/*  Below are the standard components for a "GLG4VertexGen": */


GLG4VertexGen_XXX::GLG4VertexGen_XXX(const char *arg_dbname)
  : GLG4VertexGen(arg_dbname)
{
  ...
}

GLG4VertexGen_XXX::~GLG4VertexGen_XXX()
{
  ...
}

void GLG4VertexGen_XXX::
GeneratePrimaryVertex(G4Event *argEvent)
{
  G4PrimaryVertex* vertex= new G4PrimaryVertex( 0.,0.,0.,0. );

  pDef= G4ParticleTable::GetParticleTable()->FindParticle("geantino");

  G4PrimaryParticle* particle;
  particle=
      new G4PrimaryParticle(pDef,              // particle code
			    mom.x(),           // x component of momentum
			    mom.y(),           // y component of momentum
			    mom.z()       );   // z component of momentum
  particle->SetPolarization(rpol.x(), rpol.y(), rpol.z());
  particle->SetMass(pDef->GetPDGMass()); // Geant4 is silly.

  vertex->SetPrimary( particle );

  argEvent->AddPrimaryVertex(vertex);
}


void GLG4VertexGen_XXX::
SetState(G4String newValues)
{
  newValues = util_strip_default(newValues);
  if (newValues.length() == 0) {
    // print help and current state
    RAT::info << "Current state of this GLG4VertexGen_XXX:" << newline
	   << " \"" << GetState() << "\"" << newline << newline;
    RAT::info << "Format of argument to GLG4VertexGen_XXX::SetState: " << newline
	   << newline;
    return;
  }

  ...

  std::istringstream is(newValues.c_str());
}


G4String GLG4VertexGen_XXX::
GetState()
{
  std::ostringstream os;

  ...

  G4String rv(os.str());
  return rv;
}
#endif
