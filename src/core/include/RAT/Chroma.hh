#ifndef __RAT_Chroma__
#define __RAT_Chroma__

#include <TFile.h>
#include <TTree.h>
#include <TVector3.h>

#include <G4ThreeVector.hh>
#include <memory>
#include <string>
#include <vector>

namespace RAT {

class Chroma;

// Represents the information sent to Chroma for propagataion
class PhotonData {
  friend class Chroma;

 public:
  PhotonData() : numphotons(0), event(0) {}
  virtual ~PhotonData() {}

  virtual void resize(uint32_t _numphotons);
  virtual void *fillmsg(void *dest);  // assumes dest is at least size()
  virtual void *readmsg(void *src, size_t size);
  virtual size_t size();

  uint32_t GetNumPhotons() { return numphotons; }
  std::vector<float> GetPosisitionX() { return x; }
  std::vector<float> GetPosisitionY() { return y; }
  std::vector<float> GetPosisitionZ() { return z; }
  std::vector<float> GetDirectionX() { return dx; }
  std::vector<float> GetDirectionY() { return dy; }
  std::vector<float> GetDirectionZ() { return dz; }
  std::vector<float> GetPolarizationX() { return polx; }
  std::vector<float> GetPolarizationY() { return poly; }
  std::vector<float> GetPolarizationZ() { return polz; }
  std::vector<float> GetWavelength() { return wavelength; }
  std::vector<float> GetTime() { return t; }

 protected:
  uint32_t numphotons, event;
  std::vector<float> x, y, z, dx, dy, dz, polx, poly, polz, wavelength, t;
};

class Chroma {
 public:
  // use_timeout of 0 means no timeout, otherwise timeout in seconds for propagations
  // if use_broker is true, conn is an endpoint of a Chroma broker used to find a GPU endpoint
  // otherwise conn is a GPU endpoint ready to receive PhotonData
  Chroma(const std::string filename);
  virtual ~Chroma() {}

  // appends a photon to the next propagation request
  void addPhoton(const G4ThreeVector &pos, const G4ThreeVector &dir, const G4ThreeVector &pol, const float energy,
                 const float t);

  void fillEvent();
  void writeTree();

 protected:
  PhotonData photons;

 private:
  TFile *file;
  TTree *tree;
};

}  // namespace RAT

#endif