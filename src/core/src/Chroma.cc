#include "RAT/Chroma.hh"

#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>

namespace RAT {

void PhotonData::resize(uint32_t _numphotons) {
  numphotons = _numphotons;
  x.resize(_numphotons);
  y.resize(_numphotons);
  z.resize(_numphotons);
  dx.resize(_numphotons);
  dy.resize(_numphotons);
  dz.resize(_numphotons);
  polx.resize(_numphotons);
  poly.resize(_numphotons);
  polz.resize(_numphotons);
  wavelength.resize(_numphotons);
  t.resize(_numphotons);
}

void *PhotonData::fillmsg(void *dest) {
  // TODO
  return dest;
}

void *PhotonData::readmsg(void *src, size_t size) {
  // TODO
  return src;
}

size_t PhotonData::size() {
  // FIXME: WTF is all this magic numbers?
  return 2 * sizeof(uint32_t) + 11 * sizeof(float) * numphotons;
}

void Chroma::addPhoton(const G4ThreeVector &pos, const G4ThreeVector &dir, const G4ThreeVector &pol, const float energy,
                       const float t) {
  photons.numphotons++;
  photons.x.push_back(pos.x() / CLHEP::mm);
  photons.y.push_back(pos.y() / CLHEP::mm);
  photons.z.push_back(pos.z() / CLHEP::mm);
  photons.dx.push_back(dir.x());
  photons.dy.push_back(dir.y());
  photons.dz.push_back(dir.z());
  photons.polx.push_back(pol.x());
  photons.poly.push_back(pol.y());
  photons.polz.push_back(pol.z());
  photons.wavelength.push_back((CLHEP::h_Planck * CLHEP::c_light / energy) / CLHEP::nanometer);
  photons.t.push_back(t / CLHEP::ns);
}

Chroma::Chroma(const std::string filename) {
  file = new TFile(filename.c_str(), "RECREATE");
  tree = new TTree("photons", "Photon information");
  tree->Branch("nPhotons", &(photons.numphotons));
  tree->Branch("x", &(photons.x));
  tree->Branch("y", &(photons.y));
  tree->Branch("z", &(photons.z));
  tree->Branch("u", &(photons.dx));
  tree->Branch("v", &(photons.dy));
  tree->Branch("w", &(photons.dz));
  tree->Branch("polx", &(photons.polx));
  tree->Branch("poly", &(photons.poly));
  tree->Branch("polz", &(photons.polz));
  tree->Branch("wavelength", &(photons.wavelength));
  tree->Branch("t", &(photons.t));
}

void Chroma::fillEvent() {
  tree->Fill();
  photons.numphotons = 0;
  photons.x.clear();
  photons.y.clear();
  photons.z.clear();
  photons.dx.clear();
  photons.dy.clear();
  photons.dz.clear();
  photons.polx.clear();
  photons.poly.clear();
  photons.polz.clear();
  photons.wavelength.clear();
  photons.t.clear();
}

void Chroma::writeTree() {
  tree->Write();
  tree->Print();
  file->Write();
  file->Close();
}

}  // namespace RAT