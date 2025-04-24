#include "RAT/Chroma.hh"

#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>
#include <TVector3.h>

#include "RAT/DS/PMTInfo.hh"
#include "RAT/DS/Root.hh"
#include "zmq.hpp"

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

inline static zmq::send_result_t send_string(zmq::socket_t& client, const std::string& msg,
                                             zmq::send_flags flags = zmq::send_flags::none) {
  return client.send(zmq::buffer(msg), flags);
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
  flags.clear();
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
  msg.push_back(zmq::buffer(flags));
  return zmq::send_multipart(client, msg);
}

void PEData::resize(uint32_t _num_pe) {
  numPE = _num_pe;
  channel_ids.resize(_num_pe);
  wavelengths.resize(_num_pe);
  times.resize(_num_pe);
  dx.resize(_num_pe);
  dy.resize(_num_pe);
  dz.resize(_num_pe);
  polx.resize(_num_pe);
  poly.resize(_num_pe);
  polz.resize(_num_pe);
  flags.resize(_num_pe);
}

void PEData::clear() {
  numPE = 0;
  channel_ids.clear();
  wavelengths.clear();
  times.clear();
}

bool PEData::readmsg(std::vector<zmq::message_t>& recv_msgs) {
  std::string header = recv_msgs[0].to_string();
  if (header == SIM_COMPLETE && recv_msgs.size() > PEDATA_NPARTS && *(recv_msgs[1].data<uint32_t>()) == event) {
    debug << "CHROMA-ZMQ: Received PE data!" << newline;
    copy_to_vector<uint32_t>(recv_msgs[2], channel_ids);
    copy_to_vector<float>(recv_msgs[3], dx);
    copy_to_vector<float>(recv_msgs[4], dy);
    copy_to_vector<float>(recv_msgs[5], dz);
    copy_to_vector<float>(recv_msgs[6], polx);
    copy_to_vector<float>(recv_msgs[7], poly);
    copy_to_vector<float>(recv_msgs[8], polz);
    copy_to_vector<float>(recv_msgs[9], wavelengths);
    copy_to_vector<float>(recv_msgs[10], times);
    copy_to_vector<uint32_t>(recv_msgs[11], flags);
    numPE = channel_ids.size();
    return true;
  }
  if (header == SIM_COMPLETE_ASYNC && recv_msgs.size() == 2 && *(recv_msgs[1].data<uint32_t>()) == event) {
    debug << "CHROMA-ZMQ: Sim will be done asynchrounously and written by the chroma server" << newline;
    numPE = 0;
    return true;
  }

  warn << "CHROMA-ZMQ: PE DATA RESPONSE DID NOT PASS VALIDATION!" << newline;
  warn << "Header: " << header << newline;
  return false;
}

void Chroma::addPhoton(const G4ThreeVector& pos, const G4ThreeVector& dir, const G4ThreeVector& pol, const float energy,
                       const float t, const std::string& process) {
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
  if (process == "Cerenkov") {
    photons.flags.push_back(CHERENKOV_BIT);
  } else if (process == "Scintillation") {
    photons.flags.push_back(SCINTILLATION_BIT);
  } else {
    photons.flags.push_back(0);
  }
}

Chroma::Chroma(ChromaRunMode mode, DBLinkPtr chroma_db, DS::PMTInfo* pmt_info, std::vector<PMTTime*>& pmt_time,
               std::vector<PMTCharge*>& pmt_charge)
    : runMode(mode),
      address(chroma_db->GetS("address")),
      timeout(chroma_db->GetI("timeout")),
      retries(chroma_db->GetI("retries")) {
  std::cout << "Chroma constructor callled!" << std::endl;
  if (mode == ChromaRunMode::ZMQ) {
    init_zmq(chroma_db, pmt_info, pmt_time, pmt_charge);
  } else if (mode == ChromaRunMode::ROOTFILE) {
    init_rootfile(chroma_db, pmt_info, pmt_time, pmt_charge);
  } else {
    Log::Die("CHROMA: Invalid run mode!");
  }
}

void Chroma::init_zmq(DBLinkPtr chroma_db, DS::PMTInfo* pmt_info, std::vector<PMTTime*>& pmt_time,
                      std::vector<PMTCharge*>& pmt_charge) {
  context = zmq::context_t();
  // perform ping handshake
  bool good_ping = do_send_and_recv(
      [this](zmq::socket_t& client) -> zmq::send_result_t { return send_string(client, PING); },
      [this](std::vector<zmq::message_t>& recv_msgs) -> bool { return recv_msgs[0].to_string() == ACK; });
  if (good_ping) {
    info << "CHROMA-ZMQ: PING SUCCESSFUL -- Server is online!" << newline;
    bool good_begin_run = do_send_and_recv(
        [this](zmq::socket_t& client) -> zmq::send_result_t {
          send_string(client, RUN_BEGIN, zmq::send_flags::sndmore);
          return send_string(client, DB::Get()->GetLink("IO", "ROOTProc")->GetS("default_output_filename"));
        },
        [this](std::vector<zmq::message_t>& recv_msgs) -> bool { return recv_msgs[0].to_string() == ACK; });
    if (!good_begin_run) warn << "CHROMA-ZMQ: BEGIN RUN FAILED! Server may be offline." << newline;
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
    match_chroma_and_rat_pmts();
  } else {
    Log::Die("CHROMA-ZMQ: DETECTOR INFO FAILED!");
  }
}

void Chroma::init_rootfile(DBLinkPtr chroma_db, DS::PMTInfo* pmt_info, std::vector<PMTTime*>& pmt_time,
                           std::vector<PMTCharge*>& pmt_charge) {
  rootfile = TFile::Open(chroma_db->GetS("root_file").c_str());
  if (!rootfile) {
    Log::Die("CHROMA-ROOT: ROOT FILE NOT FOUND!");
  }
  rat_pmt_info = pmt_info;
  rat_pmt_time = pmt_time;
  rat_pmt_charge = pmt_charge;
  TTree* detinfo = (TTree*)rootfile->Get("detinfo");
  if (!detinfo) {
    Log::Die("CHROMA-ROOT: DETECTOR INFO NOT FOUND!");
  }
  float px, py, pz;
  detinfo->SetBranchAddress("pmt_x", &px);
  detinfo->SetBranchAddress("pmt_y", &py);
  detinfo->SetBranchAddress("pmt_z", &pz);
  for (int i = 0; i < detinfo->GetEntries(); i++) {
    detinfo->GetEntry(i);
    pmt_x.push_back(px);
    pmt_y.push_back(py);
    pmt_z.push_back(pz);
  }

  match_chroma_and_rat_pmts();

  // setup addresses for pe data
  pe_data = (TTree*)rootfile->Get("output");
  b_event_id = pe_data->GetBranch("event_id");
  b_event_id->SetAddress(&(pes.event));
  b_nchannel_id = pe_data->GetBranch("nchannel_id");
  b_nchannel_id->SetAddress(&(pes.numPE));
  current_evt = 0;
}

void Chroma::match_chroma_and_rat_pmts() {
  rat_pmt_id.clear();
  // use PMT position and type to generate a mapping between chroma PMTs and rat PMTs
  // THere is a possible local offset. Remove this effect by subtracting positions by the difference of the averages.

  std::cout << "PMT Average positions:" << std::endl;
  TVector3 chroma_pmt_avg_pos(0, 0, 0);
  if (pmt_x.size() != rat_pmt_info->GetPMTCount()) {
    info << "Chroma PMT count: " << pmt_x.size() << newline;
    info << "RAT PMT count: " << rat_pmt_info->GetPMTCount() << newline;
    Log::Die("CHROMA-ZMQ: PMT COUNT MISMATCH!");
  }
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

  // info << "Chroma to RAT PMT Mapping: " << newline;
  // for (size_t i = 0; i < rat_pmt_id.size(); i++) {
  //   info << "Chroma PMT " << i << " -> RAT PMT " << rat_pmt_id[i] << newline;
  // }
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
  if (runMode == ChromaRunMode::ZMQ) {
    eventAction_zmq(ds);
  } else if (runMode == ChromaRunMode::ROOTFILE) {
    eventAction_rootfile(ds);
  } else {
    Log::Die("CHROMA: Invalid run mode!");
  }
}

void Chroma::eventAction_zmq(DS::Root* ds) {
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
  write_pes_to_ratds(ds);
}

void Chroma::eventAction_rootfile(DS::Root* ds) {
  b_event_id->GetEntry(current_evt);
  b_nchannel_id->GetEntry(current_evt);
  std::cout << "Event ID: " << pes.event << " NumPE: " << pes.numPE << std::endl;
  pes.resize(pes.numPE);

  pe_data->SetBranchAddress("channel_id", pes.channel_ids.data());
  pe_data->SetBranchAddress("time", pes.times.data());
  pe_data->SetBranchAddress("wavelength", pes.wavelengths.data());
  pe_data->SetBranchAddress("u", pes.dx.data());
  pe_data->SetBranchAddress("v", pes.dy.data());
  pe_data->SetBranchAddress("w", pes.dz.data());
  pe_data->SetBranchAddress("pol_x", pes.polx.data());
  pe_data->SetBranchAddress("pol_y", pes.poly.data());
  pe_data->SetBranchAddress("pol_z", pes.polz.data());
  pe_data->SetBranchAddress("flag", pes.flags.data());

  pe_data->GetEntry(current_evt);
  current_evt++;
  write_pes_to_ratds(ds);
}

void Chroma::write_pes_to_ratds(DS::Root* ds) {
  DS::MC* mc = ds->GetMC();
  mc->SetNumPE(pes.GetNumPE());
  std::unordered_map<int, int> mcpmts;
  for (size_t i = 0; i < pes.GetNumPE(); i++) {
    int chroma_ch = pes.channel_ids.at(i);
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
    mcphoton->SetLambda(pes.wavelengths.at(i) * CLHEP::nm / CLHEP::mm);
    mcphoton->SetPosition(TVector3(pmt_x[chroma_ch], pmt_y[chroma_ch], pmt_z[chroma_ch]));
    // TODO: add creation time
    TVector3 direction = TVector3(pes.dx.at(i), pes.dy.at(i), pes.dz.at(i));
    float momentum_mag = CLHEP::h_Planck * CLHEP::c_light / pes.wavelengths.at(i) * CLHEP::nm / CLHEP::MeV;
    mcphoton->SetMomentum(momentum_mag * direction);
    mcphoton->SetPolarization(TVector3(pes.polx.at(i), pes.poly.at(i), pes.polz.at(i)));
    mcphoton->SetHitTime(pes.times.at(i));
    mcphoton->SetFrontEndTime(rat_pmt_time[rat_pmt_info->GetModel(rat_ch)]->PickTime(pes.times.at(i)));
    mcphoton->SetCharge(rat_pmt_charge[rat_pmt_info->GetModel(rat_ch)]->PickCharge());
    uint32_t flag = pes.flags.at(i);
    if ((flag & BULK_REEMIT_BIT) || (flag & SURF_REEMIT_BIT)) {
      // Note: This behavior is different from RAT. RAT creates a new photon track for re-emission, whereas Chroma
      // modifies the existing photon.
      mcphoton->SetCreatorProcess("Reemission");
    } else if (flag & CHERENKOV_BIT) {
      mcphoton->SetCreatorProcess("Cerenkov");
    } else if (flag & SCINTILLATION_BIT) {
      mcphoton->SetCreatorProcess("Scintillation");
    } else {
      mcphoton->SetCreatorProcess("Unknown");
    }
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

void Chroma::endOfRun() {
  if (runMode == ChromaRunMode::ROOTFILE) {
    rootfile->Close();
  } else {
    // Signal end of run
    bool server_ack = do_send_and_recv(
        [this](zmq::socket_t& client) -> zmq::send_result_t { return send_string(client, RUN_END); },
        [this](std::vector<zmq::message_t>& recv_msgs) -> bool { return recv_msgs[0].to_string() == ACK; });
    if (server_ack) {
      info << "End of run is acknowledged." << newline;
    } else {
      warn << "End of run is signaled but not acknowledged. Server may have died." << newline;
    }
  }
}

}  // namespace RAT
