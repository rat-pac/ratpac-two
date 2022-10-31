#ifdef ZMQ_Enabled

#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>

#include <RAT/Chroma.hh>
#include <RAT/GLG4HitPhoton.hh>
#include <RAT/GLG4VEventAction.hh>

namespace RAT {

PhotonData::PhotonData() : numphotons(0), event(0) {}

PhotonData::~PhotonData() {}

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
  wavelen.resize(_numphotons);
  t.resize(_numphotons);
  trackid.resize(_numphotons);
}

void *PhotonData::fillmsg(void *dest) {
  char *mem = (char *)dest;
  *((uint32_t *)mem) = numphotons;
  mem += sizeof(uint32_t);
  *((uint32_t *)mem) = event;
  mem += sizeof(uint32_t);
  memcpy(mem, x.data(), numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(mem, y.data(), numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(mem, z.data(), numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(mem, dx.data(), numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(mem, dy.data(), numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(mem, dz.data(), numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(mem, polx.data(), numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(mem, poly.data(), numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(mem, polz.data(), numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(mem, wavelen.data(), numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(mem, t.data(), numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(mem, trackid.data(), numphotons * sizeof(uint32_t));
  mem += numphotons * sizeof(uint32_t);
  return mem;
}

void *PhotonData::readmsg(void *src) {
  char *mem = (char *)src;
  numphotons = *((uint32_t *)mem);
  mem += sizeof(uint32_t);
  resize(numphotons);
  event = *((uint32_t *)mem);
  mem += sizeof(uint32_t);
  memcpy(x.data(), mem, numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(y.data(), mem, numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(z.data(), mem, numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(dx.data(), mem, numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(dy.data(), mem, numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(dz.data(), mem, numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(polx.data(), mem, numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(poly.data(), mem, numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(polz.data(), mem, numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(wavelen.data(), mem, numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(t.data(), mem, numphotons * sizeof(double));
  mem += numphotons * sizeof(double);
  memcpy(trackid.data(), mem, numphotons * sizeof(uint32_t));
  mem += numphotons * sizeof(uint32_t);
  return mem;
}

size_t PhotonData::size() {
  return 2 * sizeof(uint32_t) + 11 * sizeof(double) * numphotons + 1 * sizeof(uint32_t) * numphotons;
}

HitData::HitData() {}

HitData::~HitData() {}

void HitData::resize(uint32_t _numphotons) {
  PhotonData::resize(_numphotons);
  channel.resize(_numphotons);
}

void *HitData::fillmsg(void *dest) {
  char *mem = (char *)PhotonData::fillmsg(dest);
  memcpy(mem, channel.data(), numphotons * sizeof(uint32_t));
  mem += numphotons * sizeof(uint32_t);
  return mem;
}

void *HitData::readmsg(void *src) {
  char *mem = (char *)PhotonData::readmsg(src);
  memcpy(channel.data(), mem, numphotons * sizeof(uint32_t));
  mem += numphotons * sizeof(uint32_t);
  return mem;
}

size_t HitData::size() { return 1 * sizeof(uint32_t) * numphotons; }

Chroma::Chroma(std::string conn) : context(1), socket(context, ZMQ_REQ) { socket.connect(conn.c_str()); }

Chroma::~Chroma() {}

void Chroma::addPhoton(const G4ThreeVector &pos, const G4ThreeVector &dir, const G4ThreeVector &pol,
                       const double energy, const double t, const uint32_t trackid) {
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
  photons.wavelen.push_back((CLHEP::h_Planck * CLHEP::c_light / energy) / CLHEP::nanometer);
  photons.t.push_back(t / CLHEP::ns);
  photons.trackid.push_back(trackid);
}

void Chroma::propagate() {
  std::cout << "Shipping off " << photons.numphotons << " photons (" << photons.size() / 1024 << " KiB)" << std::endl;

  // Build a message and send to the chroma server
  zmq::message_t request(photons.size());
  photons.fillmsg(request.data());
  socket.send(request);

  // Reset the photons object for the next round
  photons.resize(0);
  photons.event++;

  // Get reply and read out detected photon information
  zmq::message_t reply;
  socket.recv(&reply);
  hits.readmsg(reply.data());

  // Stash the hits where GLG4 code does
  for (size_t i = 0; i < hits.numphotons; i++) {
    GLG4HitPhoton *hit_photon = new GLG4HitPhoton();
    hit_photon->SetPMTID(hits.channel[i]);
    hit_photon->SetTime(hits.t[i] * CLHEP::ns);
    hit_photon->SetKineticEnergy(CLHEP::h_Planck * CLHEP::c_light / (hits.wavelen[i] * CLHEP::nanometer));
    hit_photon->SetPosition(hits.x[i] * CLHEP::mm, hits.y[i] * CLHEP::mm, hits.z[i] * CLHEP::mm);
    hit_photon->SetMomentum(hits.dx[i], hits.dy[i], hits.dz[i]);
    hit_photon->SetPolarization(hits.polx[i], hits.poly[i], hits.polz[i]);
    hit_photon->SetCount(1);
    hit_photon->SetTrackID(hits.trackid[i]);
    hit_photon->SetPrepulse(false);
    GLG4VEventAction::GetTheHitPMTCollection()->DetectPhoton(hit_photon);
  }
}

}  // namespace RAT

#endif
