#include <TFile.h>
#include <TTimeStamp.h>
#include <TTree.h>
#include <TVector3.h>

#include <RAT/DB.hh>
#include <RAT/DS/DigitPMT.hh>
#include <RAT/DS/EV.hh>
#include <RAT/DS/MC.hh>
#include <RAT/DS/MCPMT.hh>
#include <RAT/DS/MCParticle.hh>
#include <RAT/DS/MCSummary.hh>
#include <RAT/DS/PMT.hh>
#include <RAT/DS/PMTInfo.hh>
#include <RAT/DS/Root.hh>
#include <RAT/DS/Run.hh>
#include <RAT/DS/RunStore.hh>
#include <RAT/DS/WaveformAnalysisResult.hh>
#include <RAT/Log.hh>
#include <RAT/OutNtupleProc.hh>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "RAT/DS/ChannelStatus.hh"

namespace RAT {

OutNtupleProc::OutNtupleProc() : Processor("outntuple") {
  outputFile = nullptr;
  outputTree = nullptr;
  metaTree = nullptr;
  runBranch = new DS::Run();

  // Load options from the database
  DB *db = DB::Get();
  DBLinkPtr table = db->GetLink("IO", "NtupleProc");
  try {
    defaultFilename = table->GetS("default_output_filename");
    if (defaultFilename.find(".") == std::string::npos) {
      defaultFilename += ".ntuple.root";
    }
  } catch (DBNotFoundError &e) {
    defaultFilename = "output.ntuple.root";
  }
  try {
    options.tracking = table->GetZ("include_tracking");
    options.mcparticles = table->GetZ("include_mcparticles");
    options.pmthits = table->GetZ("include_pmthits");
    options.digitizerwaveforms = table->GetZ("include_digitizerwaveforms");
    options.digitizerhits = table->GetZ("include_digitizerhits");
    options.digitizerfits = table->GetZ("include_digitizerfits");
    options.untriggered = table->GetZ("include_untriggered_events");
    options.mchits = table->GetZ("include_mchits");
  } catch (DBNotFoundError &e) {
    options.tracking = false;
    options.mcparticles = false;
    options.pmthits = true;
    options.untriggered = false;
    options.mchits = true;
  }
  if (options.digitizerfits) {
    waveform_fitters = table->GetSArray("waveform_fitters");
  }
}

bool OutNtupleProc::OpenFile(std::string filename) {
  int i = 0;
  outputFile = TFile::Open(filename.c_str(), "RECREATE");
  // Meta Tree
  metaTree = new TTree("meta", "meta");
  metaTree->Branch("runId", &runId);
  metaTree->Branch("runType", &runType);
  metaTree->Branch("runTime", &runTime);
  metaTree->Branch("dsentries", &dsentries);
  metaTree->Branch("macro", &macro);
  metaTree->Branch("pmtType", &pmtType);
  metaTree->Branch("pmtId", &pmtId);
  metaTree->Branch("pmtChannel", &pmtChannel);
  metaTree->Branch("pmtIsOnline", &pmtIsOnline);
  metaTree->Branch("pmtCableOffset", &pmtCableOffset);
  metaTree->Branch("pmtX", &pmtX);
  metaTree->Branch("pmtY", &pmtY);
  metaTree->Branch("pmtZ", &pmtZ);
  metaTree->Branch("pmtU", &pmtU);
  metaTree->Branch("pmtV", &pmtV);
  metaTree->Branch("pmtW", &pmtW);
  metaTree->Branch("digitizerWindowSize", &digitizerWindowSize);
  metaTree->Branch("digitizerSampleRate_GHz", &digitizerSampleRate);
  metaTree->Branch("digitizerDynamicRange_mV", &digitizerDynamicRange);
  metaTree->Branch("digitizerResolution_mVPerADC", &digitizerVoltageResolution);
  this->AssignAdditionalMetaAddresses();
  dsentries = 0;
  // Data Tree
  outputTree = new TTree("output", "output");
  // These are the *first* particles MC positions, directions, and time
  outputTree->Branch("mcpdg", &mcpdg);
  outputTree->Branch("mcx", &mcx);
  outputTree->Branch("mcy", &mcy);
  outputTree->Branch("mcz", &mcz);
  outputTree->Branch("mcu", &mcu);
  outputTree->Branch("mcv", &mcv);
  outputTree->Branch("mcw", &mcw);
  outputTree->Branch("mcke", &mcke);
  outputTree->Branch("mct", &mct);
  // Event IDs and trigger time and nhits
  outputTree->Branch("evid", &evid);
  outputTree->Branch("subev", &subev);
  outputTree->Branch("nhits", &nhits);
  outputTree->Branch("triggerTime", &triggerTime);  // Local trigger time
  outputTree->Branch("timestamp", &timestamp);      // Global trigger time
  outputTree->Branch("timeSinceLastTrigger_us", &timeSinceLastTrigger_us);
  // MC Information
  outputTree->Branch("mcid", &mcid);
  outputTree->Branch("mcparticlecount", &mcpcount);
  outputTree->Branch("mcpecount", &mcpecount);
  outputTree->Branch("mcnhits", &mcnhits);
  outputTree->Branch("scintEdep", &scintEdep);
  outputTree->Branch("scintEdepQuenched", &scintEdepQuenched);
  // Total number of produced photons of each type
  outputTree->Branch("scintPhotons", &scintPhotons);
  outputTree->Branch("remPhotons", &remPhotons);
  outputTree->Branch("cherPhotons", &cherPhotons);
  if (options.mcparticles) {
    // Save information about *all* particles that are simulated
    // Variable naming is the same as the first particle, just plural.
    outputTree->Branch("mcpdgs", &pdgcodes);
    outputTree->Branch("mcxs", &mcPosx);
    outputTree->Branch("mcys", &mcPosy);
    outputTree->Branch("mczs", &mcPosz);
    outputTree->Branch("mcus", &mcDirx);
    outputTree->Branch("mcvs", &mcDiry);
    outputTree->Branch("mcws", &mcDirz);
    outputTree->Branch("mckes", &mcKEnergies);
    outputTree->Branch("mcts", &mcTime);
  }
  if (options.pmthits) {
    // Save full PMT hit informations
    outputTree->Branch("hitPMTID", &hitPMTID);
    // Information about *first* detected PE
    outputTree->Branch("hitPMTTime", &hitPMTTime);
    outputTree->Branch("hitPMTCharge", &hitPMTCharge);
  }
  if (options.digitizerhits) {
    // Output of the waveform analysis
    outputTree->Branch("digitNhits", &digitNhits);
    outputTree->Branch("digitPMTID", &digitPMTID);
    outputTree->Branch("digitTime", &digitTime);
    outputTree->Branch("digitCharge", &digitCharge);
    outputTree->Branch("digitNCrossings", &digitNCrossings);
    outputTree->Branch("digitPeak", &digitPeak);
    outputTree->Branch("digitLocalTriggerTime", &digitLocalTriggerTime);
  }
  if (options.digitizerfits) {
    for (const std::string &fitter_name : waveform_fitters) {
      outputTree->Branch(TString("fit_pmtid_" + fitter_name), &fitPmtID[fitter_name]);
      outputTree->Branch(TString("fit_time_" + fitter_name), &fitTime[fitter_name]);
      outputTree->Branch(TString("fit_charge_" + fitter_name), &fitCharge[fitter_name]);
    }
  }
  if (options.mchits) {
    // Save full MC PMT hit information
    outputTree->Branch("mcPMTID", &mcpmtid);
    outputTree->Branch("mcPMTNPE", &mcpmtnpe);
    outputTree->Branch("mcPMTCharge", &mcpmtcharge);

    outputTree->Branch("mcPEHitTime", &mcpehittime);
    outputTree->Branch("mcPEFrontEndTime", &mcpefrontendtime);
    // Production process
    // 1=Cherenkov, 0=Dark noise, 2=Scint., 3=Reem., 4=Unknown
    outputTree->Branch("mcPEProcess", &mcpeprocess);
    outputTree->Branch("mcPEWavelength", &mcpewavelength);
    outputTree->Branch("mcPEx", &mcpex);
    outputTree->Branch("mcPEy", &mcpey);
    outputTree->Branch("mcPEz", &mcpez);
    outputTree->Branch("mcPECharge", &mcpecharge);
  }
  if (options.tracking) {
    // Save particle tracking information
    outputTree->Branch("trackPDG", &trackPDG);
    outputTree->Branch("trackPosX", &trackPosX);
    outputTree->Branch("trackPosY", &trackPosY);
    outputTree->Branch("trackPosZ", &trackPosZ);
    outputTree->Branch("trackMomX", &trackMomX);
    outputTree->Branch("trackMomY", &trackMomY);
    outputTree->Branch("trackMomZ", &trackMomZ);
    outputTree->Branch("trackKE", &trackKE);
    outputTree->Branch("trackTime", &trackTime);
    outputTree->Branch("trackProcess", &trackProcess);
    metaTree->Branch("processCodeMap", &processCodeMap);
    outputTree->Branch("trackVolume", &trackVolume);
    metaTree->Branch("volumeCodeMap", &volumeCodeMap);
  }
  if (options.digitizerwaveforms) {
    waveformTree = new TTree("waveforms", "waveforms");
    waveformTree->Branch("evid", &evid);
    waveformTree->Branch("waveform_pmtid", &waveform_pmtid);
    waveformTree->Branch("inWindowPulseTimes", &inWindowPulseTimes);
    waveformTree->Branch("inWindowPulseCharges", &inWindowPulseCharges);
    waveformTree->Branch("waveform", &waveform);
  }
  this->AssignAdditionalAddresses();

  return true;
}

Processor::Result OutNtupleProc::DSEvent(DS::Root *ds) {
  if (!this->outputFile) {
    if (!OpenFile(this->defaultFilename.c_str())) {
      Log::Die("No output file specified");
    }
  }
  runBranch = DS::RunStore::GetRun(ds);
  DS::PMTInfo *pmtinfo = runBranch->GetPMTInfo();
  const DS::ChannelStatus *channel_status = runBranch->GetChannelStatus();
  ULong64_t stonano = 1000000000;
  dsentries++;
  // Clear the previous vectors
  pdgcodes.clear();
  mcKEnergies.clear();
  mcPosx.clear();
  mcPosy.clear();
  mcPosz.clear();
  mcDirx.clear();
  mcDiry.clear();
  mcDirz.clear();
  mcTime.clear();

  DS::MC *mc = ds->GetMC();
  mcid = mc->GetID();
  mcpcount = mc->GetMCParticleCount();
  for (int pid = 0; pid < mcpcount; pid++) {
    DS::MCParticle *particle = mc->GetMCParticle(pid);
    pdgcodes.push_back(particle->GetPDGCode());
    mcKEnergies.push_back(particle->GetKE());
    TVector3 mcpos = particle->GetPosition();
    TVector3 mcdir = particle->GetMomentum();
    mcPosx.push_back(mcpos.X());
    mcPosy.push_back(mcpos.Y());
    mcPosz.push_back(mcpos.Z());
    mcDirx.push_back(mcdir.X() / mcdir.Mag());
    mcDiry.push_back(mcdir.Y() / mcdir.Mag());
    mcDirz.push_back(mcdir.Z() / mcdir.Mag());
    mcTime.push_back(particle->GetTime());
  }
  // First particle's position, direction, and time
  mcpdg = mcpcount ? pdgcodes[0] : -9999;
  mcx = mcpcount ? mcPosx[0] : -9999;
  mcy = mcpcount ? mcPosy[0] : -9999;
  mcz = mcpcount ? mcPosz[0] : -9999;
  mcu = mcpcount ? mcDirx[0] : -9999;
  mcv = mcpcount ? mcDiry[0] : -9999;
  mcw = mcpcount ? mcDirz[0] : -9999;
  mct = mcpcount ? mcTime[0] : -9999;
  mcke = accumulate(mcKEnergies.begin(), mcKEnergies.end(), 0.0);
  // Tracking
  if (options.tracking) {
    int nTracks = mc->GetMCTrackCount();
    // Clear previous event
    trackPDG.clear();
    trackPosX.clear();
    trackPosY.clear();
    trackPosZ.clear();
    trackMomX.clear();
    trackMomY.clear();
    trackMomZ.clear();
    trackKE.clear();
    trackTime.clear();
    trackProcess.clear();
    trackVolume.clear();

    std::vector<double> xtrack, ytrack, ztrack;
    std::vector<double> pxtrack, pytrack, pztrack;
    std::vector<double> kinetic, globaltime;
    std::vector<int> processMapID;
    std::vector<int> volumeMapID;
    for (int trk = 0; trk < nTracks; trk++) {
      DS::MCTrack *track = mc->GetMCTrack(trk);
      trackPDG.push_back(track->GetPDGCode());
      xtrack.clear();
      ytrack.clear();
      ztrack.clear();
      pxtrack.clear();
      pytrack.clear();
      pztrack.clear();
      kinetic.clear();
      globaltime.clear();
      processMapID.clear();
      volumeMapID.clear();
      int nSteps = track->GetMCTrackStepCount();
      for (int stp = 0; stp < nSteps; stp++) {
        DS::MCTrackStep *step = track->GetMCTrackStep(stp);
        // Process
        std::string proc = step->GetProcess();
        if (processCodeMap.find(proc) == processCodeMap.end()) {
          processCodeMap[proc] = processCodeMap.size();
          processCodeIndex.push_back(processCodeMap.size() - 1);
          processName.push_back(proc);
        }
        // Volume
        std::string vol = step->GetVolume();
        if (volumeCodeMap.find(vol) == volumeCodeMap.end()) {
          volumeCodeMap[vol] = volumeCodeMap.size();
          volumeCodeIndex.push_back(volumeCodeMap.size() - 1);
          volumeName.push_back(vol);
        }
        volumeMapID.push_back(volumeCodeMap[vol]);
        processMapID.push_back(processCodeMap[proc]);
        TVector3 tv = step->GetEndpoint();
        TVector3 momentum = step->GetMomentum();
        kinetic.push_back(step->GetKE());
        globaltime.push_back(step->GetGlobalTime());
        xtrack.push_back(tv.X());
        ytrack.push_back(tv.Y());
        ztrack.push_back(tv.Z());
        pxtrack.push_back(momentum.X());
        pytrack.push_back(momentum.Y());
        pztrack.push_back(momentum.Z());
      }
      trackKE.push_back(kinetic);
      trackTime.push_back(globaltime);
      trackPosX.push_back(xtrack);
      trackPosY.push_back(ytrack);
      trackPosZ.push_back(ztrack);
      trackMomX.push_back(pxtrack);
      trackMomY.push_back(pytrack);
      trackMomZ.push_back(pztrack);
      trackProcess.push_back(processMapID);
      trackVolume.push_back(volumeMapID);
    }
  }

  // MCSummary info
  RAT::DS::MCSummary *mcs = mc->GetMCSummary();
  scintEdep = mcs->GetTotalScintEdep();
  scintEdepQuenched = mcs->GetTotalScintEdepQuenched();
  scintPhotons = mcs->GetNumScintPhoton();
  remPhotons = mcs->GetNumReemitPhoton();
  cherPhotons = mcs->GetNumCerenkovPhoton();

  // MCPMT information
  mcpmtid.clear();
  mcpmtnpe.clear();
  mcpmtcharge.clear();

  // MCPE information
  mcpehittime.clear();
  mcpefrontendtime.clear();
  mcpeprocess.clear();
  mcpewavelength.clear();
  mcpex.clear();
  mcpey.clear();
  mcpez.clear();
  mcpecharge.clear();

  mcnhits = mc->GetMCPMTCount();
  mcpecount = mc->GetNumPE();
  if (options.mchits) {
    for (int ipmt = 0; ipmt < mc->GetMCPMTCount(); ipmt++) {
      DS::MCPMT *mcpmt = mc->GetMCPMT(ipmt);
      mcpmtid.push_back(mcpmt->GetID());
      mcpmtnpe.push_back(mcpmt->GetMCPhotonCount());
      mcpmtcharge.push_back(mcpmt->GetCharge());
      TVector3 position = pmtinfo->GetPosition(mcpmt->GetID());
      for (int ipe = 0; ipe < mcpmt->GetMCPhotonCount(); ipe++) {
        RAT::DS::MCPhoton *mcph = mcpmt->GetMCPhoton(ipe);
        mcpehittime.push_back(mcph->GetHitTime());
        mcpefrontendtime.push_back(mcph->GetFrontEndTime());
        mcpewavelength.push_back(mcph->GetLambda());
        mcpex.push_back(position.X());
        mcpey.push_back(position.Y());
        mcpez.push_back(position.Z());
        mcpecharge.push_back(mcph->GetCharge());
        if (mcph->IsDarkHit()) {
          mcpeprocess.push_back(noise);
          continue;
        }
        std::string process = mcph->GetCreatorProcess();
        if (process.find("Cerenkov") != std::string::npos) {
          mcpeprocess.push_back(cherenkov);
        } else if (process.find("Scintillation") != std::string::npos) {
          mcpeprocess.push_back(scintillation);
        } else if (process.find("Reemission") != std::string::npos) {
          mcpeprocess.push_back(reemission);
        } else {
          mcpeprocess.push_back(unknown);
        }
      }
    }
  }

  // EV Branches
  for (subev = 0; subev < ds->GetEVCount(); subev++) {
    DS::EV *ev = ds->GetEV(subev);
    evid = ev->GetID();
    triggerTime = ev->GetCalibratedTriggerTime();
    timestamp = (ev->GetUTC().GetSec() - runBranch->GetStartTime().GetSec()) * 1e9 +
                (ev->GetUTC().GetNanoSec() - runBranch->GetStartTime().GetNanoSec()) + triggerTime;
    timeSinceLastTrigger_us = ev->GetDeltaT() / 1000.;
    auto fitVector = ev->GetFitResults();
    std::map<std::string, double *> fitvalues;
    std::map<std::string, bool *> fitvalids;
    std::map<std::string, int *> intFOMs;
    std::map<std::string, bool *> boolFOMs;
    std::map<std::string, double *> doubleFOMs;
    for (auto fit : fitVector) {
      std::string name = fit->GetFitterName();
      // Check the validity and write it out
      if (fit->GetEnablePosition()) {
        TVector3 pos = fit->GetPosition();
        fitvalues["x_" + name] = new double(pos.X());
        fitvalues["y_" + name] = new double(pos.Y());
        fitvalues["z_" + name] = new double(pos.Z());
        fitvalids["validposition_" + name] = new bool(fit->GetValidPosition());
      }
      if (fit->GetEnableDirection()) {
        TVector3 dir = fit->GetDirection();
        fitvalues["u_" + name] = new double(dir.X());
        fitvalues["v_" + name] = new double(dir.Y());
        fitvalues["w_" + name] = new double(dir.Z());
        fitvalids["validdirection_" + name] = new bool(fit->GetValidDirection());
      }
      if (fit->GetEnableEnergy()) {
        fitvalues["energy_" + name] = new double(fit->GetEnergy());
        fitvalids["validenergy" + name] = new bool(fit->GetValidEnergy());
      }
      if (fit->GetEnableTime()) {
        fitvalues["time_" + name] = new double(fit->GetTime());
        fitvalids["validtime" + name] = new bool(fit->GetValidTime());
      }
      // Figures of merit > 3 types
      for (auto const &[label, value] : fit->boolFiguresOfMerit) {
        boolFOMs[label + "_" + name] = new bool(value);
      }
      for (auto const &[label, value] : fit->intFiguresOfMerit) {
        intFOMs[label + "_" + name] = new int(value);
      }
      for (auto const &[label, value] : fit->doubleFiguresOfMerit) {
        doubleFOMs[label + "_" + name] = new double(value);
      }
    }
    // Write fitter values into TTree
    for (auto const &[label, value] : fitvalues) {
      this->SetBranchValue(label, value);
    }
    for (auto const &[label, value] : fitvalids) {
      this->SetBranchValue(label, value);
    }
    for (auto const &[label, value] : intFOMs) {
      this->SetBranchValue(label, value);
    }
    for (auto const &[label, value] : boolFOMs) {
      this->SetBranchValue(label, value);
    }
    for (auto const &[label, value] : doubleFOMs) {
      this->SetBranchValue(label, value);
    }
    nhits = ev->GetPMTCount();
    if (options.pmthits) {
      hitPMTID.clear();
      hitPMTTime.clear();
      hitPMTCharge.clear();

      for (int pmtc : ev->GetAllPMTIDs()) {
        RAT::DS::PMT *pmt = ev->GetOrCreatePMT(pmtc);
        hitPMTID.push_back(pmt->GetID());
        hitPMTTime.push_back(pmt->GetTime());
        hitPMTCharge.push_back(pmt->GetCharge());
      }
    }
    if (ev->DigitizerExists()) {
      // Writing this information to meta, despite the fact that in principle events could have
      // varying values for these fields. Writing these values to meta is much more convenient
      // to use, so I'm going to assume that this doesn't happen;)
      DS::Digit digitizer = ev->GetDigitizer();
      digitizerWindowSize = digitizer.GetNSamples();
      digitizerSampleRate = digitizer.GetSamplingRate();
      digitizerDynamicRange = digitizer.GetDynamicRange();
      digitizerVoltageResolution = digitizer.GetVoltageResolution();
    }
    if (options.digitizerhits) {
      digitNhits = 0;
      digitTime.clear();
      digitCharge.clear();
      digitNCrossings.clear();
      digitPeak.clear();
      digitPMTID.clear();
      digitLocalTriggerTime.clear();

      if (options.digitizerfits) {
        for (const std::string &fitter_name : waveform_fitters) {
          // construct arrays for all fitters
          fitPmtID[fitter_name].clear();
          fitTime[fitter_name].clear();
          fitCharge[fitter_name].clear();
        }
      }

      for (int pmtc : ev->GetAllDigitPMTIDs()) {
        RAT::DS::DigitPMT *digitpmt = ev->GetOrCreateDigitPMT(pmtc);
        digitPMTID.push_back(digitpmt->GetID());
        digitTime.push_back(digitpmt->GetDigitizedTime());
        digitCharge.push_back(digitpmt->GetDigitizedCharge());
        if (digitpmt->GetNCrossings() > 0) {
          digitNhits++;
        }
        digitNCrossings.push_back(digitpmt->GetNCrossings());
        digitPeak.push_back(digitpmt->GetPeakVoltage());
        digitLocalTriggerTime.push_back(digitpmt->GetLocalTriggerTime());
        if (options.digitizerfits) {
          const std::vector<std::string> fitters = digitpmt->GetFitterNames();
          for (std::string fitter_name : fitters) {
            DS::WaveformAnalysisResult *fit_result = digitpmt->GetOrCreateWaveformAnalysisResult(fitter_name);
            for (int hitidx = 0; hitidx < fit_result->getNhits(); hitidx++) {
              fitPmtID[fitter_name].push_back(digitpmt->GetID());
              fitTime[fitter_name].push_back(fit_result->getTime(hitidx));
              fitCharge[fitter_name].push_back(fit_result->getCharge(hitidx));
              // TODO: figures of merit -- you probably need some nested map
            }
          }
        }
      }
    }
    if (options.digitizerwaveforms) {
      DS::Digit digitizer = ev->GetDigitizer();
      double readout_window_min = 0;
      double readout_window_max = digitizer.GetNSamples() * digitizer.GetTimeStepNS();
      for (auto const &pair : digitizer.GetAllWaveforms()) {
        waveform_pmtid = pair.first;
        waveform = pair.second;
        inWindowPulseTimes.clear();
        inWindowPulseCharges.clear();
        if (mc->GetMCPMTCount() == 0) {
        }  // if there's no MC information, skip it
        else if (waveform_pmtid < 0) {
        }  // these are nonPMT channels. No MC info
        else {
          auto it = std::find(mcpmtid.begin(), mcpmtid.end(), waveform_pmtid);
          if (it == mcpmtid.end())
            warn << "No MC information found for PMTID = " << waveform_pmtid
                 << " but waveform exists for some reason..." << newline;
          else {
            double time_offset = channel_status->GetCableOffsetByPMTID(waveform_pmtid);
            DS::MCPMT *mcpmt = mc->GetMCPMT(it - mcpmtid.begin());
            for (int ipe = 0; ipe < mcpmt->GetMCPhotonCount(); ipe++) {
              DS::MCPhoton *mcph = mcpmt->GetMCPhoton(ipe);
              Double_t time = mcph->GetFrontEndTime() - ev->GetCalibratedTriggerTime() + time_offset;
              Double_t charge = mcph->GetCharge();
              if (time > readout_window_min && time < readout_window_max) {
                inWindowPulseTimes.push_back(time);
                inWindowPulseCharges.push_back(charge);
              }
            }  // END loop over mcphotons
          }
        }  // END IF
        waveformTree->Fill();
      }
    }
    this->FillEvent(ds, ev);
    outputTree->Fill();
  }
  if (options.untriggered && ds->GetEVCount() == 0) {
    // EV information
    evid = -1;
    subev = -1;
    nhits = -1;
    triggerTime = 0;
    timeSinceLastTrigger_us = 0;
    if (options.pmthits) {
      hitPMTID.clear();
      hitPMTTime.clear();
      hitPMTCharge.clear();
    }
    if (options.digitizerhits) {
      digitNhits = 0;
      digitTime.clear();
      digitCharge.clear();
      digitNCrossings.clear();
      digitPeak.clear();
      digitPMTID.clear();
      digitLocalTriggerTime.clear();
      if (options.digitizerfits) {
        for (const std::string &fitter_name : waveform_fitters) {
          // construct arrays for all fitters
          fitPmtID[fitter_name].clear();
          fitTime[fitter_name].clear();
          fitCharge[fitter_name].clear();
        }
      }
    }
    this->FillNoTriggerEvent(ds);
    outputTree->Fill();
  }

  // FIX THE ABOVE
  // int errorcode = outputTree->Fill();
  // if( errorcode < 0 )
  //{
  //  Log::Die(std::string("OutNtupleProc: Error fill ttree, check disk
  //  space"));
  //}
  return Processor::OK;
}

OutNtupleProc::~OutNtupleProc() {
  if (outputFile) {
    outputFile->cd();

    DS::PMTInfo *pmtinfo = runBranch->GetPMTInfo();
    const DS::ChannelStatus *ch_status = runBranch->GetChannelStatus();
    for (int id = 0; id < pmtinfo->GetPMTCount(); id++) {
      int type = pmtinfo->GetType(id);
      int channel = pmtinfo->GetChannelNumber(id);
      TVector3 position = pmtinfo->GetPosition(id);
      TVector3 direction = pmtinfo->GetDirection(id);
      pmtType.push_back(type);
      pmtId.push_back(id);
      pmtChannel.push_back(channel);
      pmtIsOnline.push_back(ch_status->GetOnlineByPMTID(id));
      pmtCableOffset.push_back(ch_status->GetCableOffsetByPMTID(id));
      pmtX.push_back(position.X());
      pmtY.push_back(position.Y());
      pmtZ.push_back(position.Z());
      pmtU.push_back(direction.X());
      pmtV.push_back(direction.Y());
      pmtW.push_back(direction.Z());
    }
    runId = runBranch->GetID();
    runType = runBranch->GetType();
    // Converting to unix time
    ULong64_t stonano = 1000000000;
    TTimeStamp rootTime = runBranch->GetStartTime();
    runTime = static_cast<ULong64_t>(rootTime.GetSec()) * stonano + static_cast<ULong64_t>(rootTime.GetNanoSec());
    macro = Log::GetMacro();
    metaTree->Fill();
    metaTree->Write();
    outputTree->Write();
    if (options.digitizerwaveforms) waveformTree->Write();
    /*
    TMap* dbtrace = Log::GetDBTraceMap();
    dbtrace->Write("db", TObject::kSingleKey);
    */
    // outputFile->Write(0, TObject::kOverwrite);
    outputFile->Close();
    delete outputFile;
  }
}

void OutNtupleProc::SetS(std::string param, std::string value) {
  if (param == "file") {
    this->defaultFilename = value;
  }
}

void OutNtupleProc::SetI(std::string param, int value) {
  if (param == "include_tracking") {
    options.tracking = value ? true : false;
  }
  if (param == "include_mcparticles") {
    options.mcparticles = value ? true : false;
  }
  if (param == "include_pmthits") {
    options.pmthits = value ? true : false;
  }
  if (param == "include_untriggered_events") {
    options.untriggered = value ? true : false;
  }
  if (param == "include_mchits") {
    options.mchits = value ? true : false;
  }
  if (param == "include_digitizerwaveforms") {
    options.digitizerwaveforms = value ? true : false;
  }
  if (param == "include_digitizerhits") {
    options.digitizerhits = value ? true : false;
  }
  if (param == "include_digitizerfits") {
    options.digitizerfits = value ? true : false;
  }
}

}  // namespace RAT
