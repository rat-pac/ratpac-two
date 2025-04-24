#ifndef __RAT_Chroma__
#define __RAT_Chroma__

#include <TClassTable.h>
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
#define ACK "ACK"
#define RUN_BEGIN "RUN_BEGIN"
#define RUN_END "RUN_END"
#define PHOTONDATA "PHOTONDATA"
#define NO_WORKERS "NO_WORKERS"
#define SIM_FAILED "SIM_FAILED"
#define UNKNOWN_REQUEST "UNKNOWN_REQUEST"
#define SIM_COMPLETE "SIM_COMPLETE"
#define SIM_COMPLETE_ASYNC "SIM_COMPLETE_ASYNC"
#define DETECTOR_INFO "DETECTOR_INFO"
#define PEDATA_NPARTS 11
#define CHERENKOV_BIT (0x1 << 10)
#define SCINTILLATION_BIT (0x1 << 11)
#define BULK_REEMIT_BIT (0x1 << 9)
#define SURF_REEMIT_BIT (0x1 << 7)
namespace RAT {

enum class ChromaRunMode { ZMQ, ROOTFILE };

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
  std::vector<uint32_t> GetFlags() { return flags; }
  std::vector<float> GetTime() { return t; }

 protected:
  uint32_t numphotons, event;
  std::vector<float> x, y, z, dx, dy, dz, polx, poly, polz, wavelength, t;
  std::vector<uint32_t> flags;
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
  std::vector<uint32_t> channel_ids, flags;
  std::vector<float> times, wavelengths, dx, dy, dz, polx, poly, polz;
};

class Chroma {
 public:
  // use_timeout of 0 means no timeout, otherwise timeout in seconds for propagations
  // if use_broker is true, conn is an endpoint of a Chroma broker used to find a GPU endpoint
  // otherwise conn is a GPU endpoint ready to receive PhotonData
  Chroma(ChromaRunMode, DBLinkPtr, DS::PMTInfo *, std::vector<PMTTime *> &, std::vector<PMTCharge *> &);
  virtual ~Chroma() {}

  // appends a photon to the next propagation request
  void addPhoton(const G4ThreeVector &pos, const G4ThreeVector &dir, const G4ThreeVector &pol, const float energy,
                 const float t, const std::string &process);
  void setEventID(const G4int evtid) { photons.event = evtid; }
  void eventAction(DS::Root *);
  void endOfRun();

  ChromaRunMode runMode;

 protected:
  PhotonData photons;
  PEData pes;

 private:
  zmq::socket_t s_client_socket();
  void init_zmq(DBLinkPtr, DS::PMTInfo *, std::vector<PMTTime *> &, std::vector<PMTCharge *> &);
  void init_rootfile(DBLinkPtr, DS::PMTInfo *, std::vector<PMTTime *> &, std::vector<PMTCharge *> &);
  void eventAction_zmq(DS::Root *);
  void eventAction_rootfile(DS::Root *);
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
  void match_chroma_and_rat_pmts();
  void write_pes_to_ratds(DS::Root *);

  zmq::context_t context;

  const std::string address;
  const size_t retries;
  const std::chrono::milliseconds timeout;

  std::vector<float> pmt_x, pmt_y, pmt_z;
  DS::PMTInfo *rat_pmt_info;
  std::vector<PMTTime *> rat_pmt_time;
  std::vector<PMTCharge *> rat_pmt_charge;
  std::vector<int> rat_pmt_id;  // rat_pmt_id[chroma_pmt_id] = rat_pmt_id

  size_t current_evt;
  TFile *rootfile;
  TTree *pe_data;
  TBranch *b_event_id;
  TBranch *b_nchannel_id;
  // DBLinkPtr chroma_db;
};

}  // namespace RAT

#endif
