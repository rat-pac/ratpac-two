#ifndef __RAT_Chroma__
#define __RAT_Chroma__

#include <TFile.h>
#include <TTree.h>
#include <TVector3.h>

#include <G4ThreeVector.hh>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "RAT/DB.hh"
#include "RAT/DS/Root.hh"
#include "RAT/DS/Run.hh"
#include "RAT/Log.hh"
#include "RAT/PDFPMTCharge.hh"
#include "RAT/PDFPMTTime.hh"
#include "zmq.hpp"
#include "zmq_addon.hpp"

#define PING "PING"
#define PONG "PONG"
#define PHOTONDATA "PHOTONDATA"
#define NO_WORKERS "NO_WORKERS"
#define SIM_FAILED "SIM_FAILED"
#define UNKNOWN_REQUEST "UNKNOWN_REQUEST"
#define SIM_COMPLETE "SIM_COMPLETE"
#define DETECTOR_INFO "DETECTOR_INFO"
namespace RAT {

class Chroma;

// Represents the information sent to Chroma for propagataion
class PhotonData {
  friend class Chroma;

 public:
  PhotonData() : numphotons(0), event(0) {}
  virtual ~PhotonData() {}

  virtual void resize(uint32_t _numphotons);
  zmq::send_result_t send(zmq::socket_t &client);
  // virtual void *readmsg(void *src, size_t size);
  // virtual size_t size();
  virtual void clear();

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

class PEData {
  friend class Chroma;

 public:
  PEData() : numPE(0), event(0) {}
  virtual ~PEData() {}

  virtual void resize(uint32_t _num_pe);
  virtual bool readmsg(std::vector<zmq::message_t> &);
  virtual void clear();

  uint32_t GetNumPE() { return numPE; }
  std::vector<uint32_t> GetChannelIDs() { return channel_ids; }
  std::vector<float> GetWavelengths() { return wavelengths; }
  std::vector<float> GetTimes() { return times; }

 protected:
  uint32_t numPE, event;
  std::vector<uint32_t> channel_ids;
  std::vector<float> times, wavelengths;
};

class Chroma {
 public:
  // use_timeout of 0 means no timeout, otherwise timeout in seconds for propagations
  // if use_broker is true, conn is an endpoint of a Chroma broker used to find a GPU endpoint
  // otherwise conn is a GPU endpoint ready to receive PhotonData
  Chroma(DBLinkPtr, DS::PMTInfo *, std::vector<PMTTime *> &, std::vector<PMTCharge *> &);
  virtual ~Chroma() {}

  // appends a photon to the next propagation request
  void addPhoton(const G4ThreeVector &pos, const G4ThreeVector &dir, const G4ThreeVector &pol, const float energy,
                 const float t);
  void setEventID(const G4int evtid) { photons.event = evtid; }

  void eventAction(DS::Root *);
  void endOfRun();

 protected:
  PhotonData photons;
  PEData pes;

 private:
  zmq::socket_t s_client_socket();

  /**
   * @brief Perform a send action, with the proper error handling, retries, and timeouts.
   * Process and validate the server reply.
   * @arg send_fn A function that sends a message to the server. Takes the socket used to send the message as an
   * argument.
   * @arg recv_fn A function that processes the server reply. Takes a vector of messages as an argument,
   *        returns true if the reply is valid.
   * @return
   */
  bool do_send_and_recv(std::function<zmq::send_result_t(zmq::socket_t &)>,
                        std::function<bool(std::vector<zmq::message_t> &)>);
  bool process_detinfo_reply(std::vector<zmq::message_t> &);

  zmq::context_t context;

  const std::string address;
  const size_t retries;
  const std::chrono::milliseconds timeout;

  std::vector<float> pmt_x, pmt_y, pmt_z;
  DS::PMTInfo *rat_pmt_info;
  std::vector<PMTTime *> rat_pmt_time;
  std::vector<PMTCharge *> rat_pmt_charge;
  std::vector<int> rat_pmt_id;  // rat_pmt_id[chroma_pmt_id] = rat_pmt_id

  // DBLinkPtr chroma_db;
};

}  // namespace RAT

#endif
