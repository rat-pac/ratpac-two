#include "RAT/Chroma.hh"

#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>
#include <TMap.h>
#include <TVector3.h>

#include "RAT/DS/PMTInfo.hh"

inline static std::string s_recv(zmq::socket_t& socket, int flags = 0) {
  zmq::message_t message;
  auto recv_flags = (flags == 0) ? zmq::recv_flags::none : zmq::recv_flags::dontwait;
  (void)socket.recv(message, recv_flags);

  return std::string(static_cast<const char*>(message.data()), message.size());
}

template <typename T>
inline static void copy_to_vector(zmq::message_t& msg, std::vector<T>& dest) {
  dest.resize(msg.size() / sizeof(T));
  std::memcpy(dest.data(), msg.data(), msg.size());
}

inline static zmq::send_result_t send_string(zmq::socket_t& client, const std::string& msg) {
  return client.send(zmq::buffer(msg), zmq::send_flags::none);
}

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

void PhotonData::clear() {
  numphotons = 0;
  x.clear();
  y.clear();
  z.clear();
  dx.clear();
  dy.clear();
  dz.clear();
  polx.clear();
  poly.clear();
  polz.clear();
  wavelength.clear();
  t.clear();
}

zmq::send_result_t PhotonData::send(zmq::socket_t& client) {
  debug << "CHROMA-ZMQ: Sending photon data to Chroma..." << newline;
  std::vector<zmq::mutable_buffer> msg;
  std::string header = PHOTONDATA;
  msg.push_back(zmq::buffer(header));
  std::vector<uint32_t> metadata = {event, numphotons};
  msg.push_back(zmq::buffer(metadata));
  msg.push_back(zmq::buffer(x));
  msg.push_back(zmq::buffer(y));
  msg.push_back(zmq::buffer(z));
  msg.push_back(zmq::buffer(dx));
  msg.push_back(zmq::buffer(dy));
  msg.push_back(zmq::buffer(dz));
  msg.push_back(zmq::buffer(polx));
  msg.push_back(zmq::buffer(poly));
  msg.push_back(zmq::buffer(polz));
  msg.push_back(zmq::buffer(wavelength));
  msg.push_back(zmq::buffer(t));
  return zmq::send_multipart(client, msg);
}

void PEData::resize(uint32_t _num_pe) {
  numPE = _num_pe;
  channel_ids.resize(_num_pe);
  wavelengths.resize(_num_pe);
  times.resize(_num_pe);
}

void PEData::clear() {
  numPE = 0;
  channel_ids.clear();
  wavelengths.clear();
  times.clear();
}

bool PEData::readmsg(std::vector<zmq::message_t>& recv_msgs) {
  std::string header = recv_msgs[0].to_string();
  if (header == SIM_COMPLETE && recv_msgs.size() > 4 && *(recv_msgs[1].data<uint32_t>()) == event) {
    debug << "CHROMA-ZMQ: Received PE data!" << newline;
    copy_to_vector<uint32_t>(recv_msgs[2], channel_ids);
    copy_to_vector<float>(recv_msgs[3], times);
    copy_to_vector<float>(recv_msgs[4], wavelengths);
    numPE = channel_ids.size();
    return true;
  }
  warn << "CHROMA-ZMQ: PE DATA RESPONSE DID NOT PASS VALIDATION!" << newline;
  warn << "Header: " << header << newline;
  return false;
}

void Chroma::addPhoton(const G4ThreeVector& pos, const G4ThreeVector& dir, const G4ThreeVector& pol, const float energy,
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

Chroma::Chroma(DBLinkPtr chroma_db, DS::PMTInfo* pmt_info, std::vector<PMTTime*>& pmt_time,
               std::vector<PMTCharge*>& pmt_charge)
    : address(chroma_db->GetS("address")), timeout(chroma_db->GetI("timeout")), retries(chroma_db->GetI("retries")) {
  context = zmq::context_t();
  // perform ping handshake
  bool good_ping = do_send_and_recv(
      [this](zmq::socket_t& client) -> zmq::send_result_t { return send_string(client, PING); },
      [this](std::vector<zmq::message_t>& recv_msgs) -> bool { return recv_msgs[0].to_string() == PONG; });
  if (good_ping) {
    info << "CHROMA-ZMQ: PING SUCCESSFUL -- Server is online!" << newline;
  } else {
    Log::Die("CHROMA-ZMQ: PING FAILED -- No response from server!");
  }

  // get detector info from chroma
  bool good_detinfo = do_send_and_recv(
      [this](zmq::socket_t& client) -> zmq::send_result_t { return send_string(client, DETECTOR_INFO); },
      [this](std::vector<zmq::message_t>& recv_msgs) -> bool { return process_detinfo_reply(recv_msgs); });
  if (good_detinfo) {
    info << "CHROMA-ZMQ: DETECTOR INFO SUCCESSFUL!" << newline;
    rat_pmt_info = pmt_info;
    rat_pmt_time = pmt_time;
    rat_pmt_charge = pmt_charge;
    rat_pmt_id.clear();
    // use PMT position and type to generate a mapping between chroma PMTs and rat PMTs
    // THere is a possible local offset. Remove this effect by subtracting positions by the difference of the averages.

    std::cout << "PMT Average positions:" << std::endl;
    TVector3 chroma_pmt_avg_pos(0, 0, 0);
    for (size_t i = 0; i < pmt_x.size(); i++) {
      chroma_pmt_avg_pos += TVector3(pmt_x[i], pmt_y[i], pmt_z[i]);
    }
    chroma_pmt_avg_pos *= (1.0 / pmt_x.size());

    TVector3 rat_pmt_avg_pos(0, 0, 0);
    for (size_t i = 0; i < rat_pmt_info->GetPMTCount(); i++) {
      rat_pmt_avg_pos += rat_pmt_info->GetPosition(i);
    }
    rat_pmt_avg_pos *= (1.0 / rat_pmt_info->GetPMTCount());
    TVector3 offset = rat_pmt_avg_pos - chroma_pmt_avg_pos;
    info << "Detected a local offset: ( " << offset.x() << ", " << offset.y() << ", " << offset.z() << " )" << newline;
    // Yes, this is O(n^2), but do you want to figure out how to put TVectors as a key in a hashmap? Yeah, me neither.
    for (size_t i = 0; i < pmt_x.size(); i++) {
      TVector3 chroma_pmt_pos(pmt_x[i], pmt_y[i], pmt_z[i]);
      chroma_pmt_pos += offset;
      bool match_found = false;
      for (int j = 0; j < rat_pmt_info->GetPMTCount(); j++) {
        TVector3 rat_pmt_pos = rat_pmt_info->GetPosition(j);
        if ((rat_pmt_pos - chroma_pmt_pos).Mag() < 1e-2) {
          rat_pmt_id.push_back(j);
          match_found = true;
          break;
        }
      }
      if (!match_found) {
        warn << "CHROMA-ZMQ: PMT MAPPING FAILED!" << newline;
      }
    }

    std::cout << "Chroma to RAT PMT Mapping: " << std::endl;
    for (size_t i = 0; i < rat_pmt_id.size(); i++) {
      info << "Chroma PMT " << i << " -> RAT PMT " << rat_pmt_id[i] << newline;
    }
  } else {
    Log::Die("CHROMA-ZMQ: DETECTOR INFO FAILED!");
  }
}

zmq::socket_t Chroma::s_client_socket() {
  zmq::socket_t client(context, ZMQ_REQ);
  client.connect(address);
  //  Configure socket to not wait at close time
  int linger = 0;
  client.set(zmq::sockopt::linger, linger);
  return client;
}

void Chroma::eventAction(DS::Root* ds) {
  pes.event = photons.event;
  bool good_reply =
      do_send_and_recv([this](zmq::socket_t& client) -> zmq::send_result_t { return photons.send(client); },
                       [this](std::vector<zmq::message_t>& recv_msgs) -> bool { return pes.readmsg(recv_msgs); });
  if (!good_reply) {
    warn << "CHROMA-ZMQ: Failed to receive valid response from server!" << newline;
  }
  if (rat_pmt_info == nullptr) {
    Log::Die("CHROMA-ZMQ: PMT INFO NOT FOUND!");
  }
  DS::MC* mc = ds->GetMC();
  mc->SetNumPE(pes.GetNumPE());
  std::unordered_map<int, int> mcpmts;
  for (size_t i = 0; i < pes.GetNumPE(); i++) {
    int chroma_ch = pes.channel_ids[i];
    int rat_ch = rat_pmt_id[chroma_ch];
    if (mcpmts.find(rat_ch) == mcpmts.end()) {
      DS::MCPMT* mcpmt = mc->AddNewMCPMT();
      mcpmts[rat_ch] = mc->GetMCPMTCount() - 1;
      mcpmt->SetID(rat_ch);
      mcpmt->SetType(rat_pmt_info->GetType(rat_ch));
    }
    DS::MCPMT* mcpmt = mc->GetMCPMT(mcpmts[rat_ch]);
    DS::MCPhoton* mcphoton = mcpmt->AddNewMCPhoton();
    mcphoton->SetDarkHit(false);
    mcphoton->SetAfterPulse(false);
    mcphoton->SetLambda(pes.wavelengths[i]);
    mcphoton->SetPosition(TVector3(pmt_x[chroma_ch], pmt_y[chroma_ch], pmt_z[chroma_ch]));
    // TODO: add momentum, polarization, creation time, creatorprocess
    mcphoton->SetHitTime(pes.times[i]);
    mcphoton->SetFrontEndTime(rat_pmt_time[rat_pmt_info->GetModel(rat_ch)]->PickTime(pes.times[i]));
    mcphoton->SetCharge(rat_pmt_charge[rat_pmt_info->GetModel(rat_ch)]->PickCharge());
  }
  /////////////////////
  photons.clear();
  pes.clear();
}

bool Chroma::process_detinfo_reply(std::vector<zmq::message_t>& recv_msgs) {
  std::string header = recv_msgs[0].to_string();
  if (header == DETECTOR_INFO) {
    info << "CHROMA-ZMQ: Received detector info!" << newline;
    copy_to_vector<float>(recv_msgs[1], pmt_x);
    copy_to_vector<float>(recv_msgs[2], pmt_y);
    copy_to_vector<float>(recv_msgs[3], pmt_z);
    std::vector<int32_t> pmt_type;
    copy_to_vector<int32_t>(recv_msgs[4], pmt_type);
    return true;
  }
  warn << "CHROMA-ZMQ: DETECTOR INFO RESPONSE DID NOT PASS VALIDATION!" << newline;
  return false;
}

bool Chroma::do_send_and_recv(std::function<zmq::send_result_t(zmq::socket_t&)> send_fn,
                              std::function<bool(std::vector<zmq::message_t>&)> recv_fn) {
  zmq::socket_t client = Chroma::s_client_socket();
  size_t retries_left = retries;
  if (!send_fn(client)) warn << "Send has failed!" << newline;

  bool expect_reply = true;
  bool valid_response = false;
  while (expect_reply) {
    zmq::pollitem_t items[] = {{client, 0, ZMQ_POLLIN, 0}};
    zmq::poll(&items[0], 1, timeout);
    retries_left--;
    if (items[0].revents & ZMQ_POLLIN) {  // received reply
      std::vector<zmq::message_t> recv_msgs;
      const zmq::recv_result_t recv_flag = zmq::recv_multipart(client, std::back_inserter(recv_msgs));
      if (!recv_flag) warn << "RECV FAILED!" << newline;
      valid_response = recv_fn(recv_msgs);
      if (valid_response) {
        expect_reply = false;
      } else {
        warn << "did not receive valid response, retrying..." << newline;
      }
    } else if (retries_left == 0) {
      warn << "server seems to be offline, abandoning..." << newline;
      expect_reply = false;
    } else {
      warn << "no response from server, retrying..." << newline;
      // old socket is confused; close it and open a new one
      client = Chroma::s_client_socket();
      // send message again
      if (!send_fn(client)) warn << "Send has failed!" << newline;
    }
  }
  return valid_response;
}

void Chroma::endOfRun() {}

}  // namespace RAT
