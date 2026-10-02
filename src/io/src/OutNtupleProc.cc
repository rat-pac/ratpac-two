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
#include <set>
#include <sstream>
#include <stlplus/string_utilities.hpp>
#include <string>
#include <vector>

#include "RAT/DS/ChannelStatus.hh"

namespace RAT {

OutNtupleProc::OutNtupleProc() : Processor("outntuple") {
  outputFile = nullptr;
  outputTree = nullptr;
  metaTree = nullptr;
  runBranch = new DS::Run();
  done_writing_calib = false;

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
    options.nthits = table->GetZ("include_nestedtubehits");
    options.calib = table->GetZ("include_calib");
    options.transittime = table->GetZ("include_mctransittime");
  } catch (DBNotFoundError &e) {
    options.tracking = false;
    options.mcparticles = false;
    options.pmthits = true;
    options.untriggered = false;
    options.mchits = true;
    options.calib = true;
    options.nthits = false;
  }
  if (options.digitizerfits) {
    waveform_fitters = table->GetSArray("waveform_fitters");
    for (const std::string &fitter_name : waveform_fitters) {
      try {
        waveform_fitter_FOMs[fitter_name] = table->GetSArray("waveform_fitter_FOM_" + fitter_name);
      } catch (DBNotFoundError &e) {
        waveform_fitter_FOMs[fitter_name].clear();
      }
    }
  }
  event_fitters = table->GetSArray("event_fitters");
  for (const std::string &full_name : event_fitters) {
    if (event_fitter_FOMs.find(full_name) != event_fitter_FOMs.end()) continue;
    std::set<std::string> fom_set;
    // Load FOMs declared under the base fitter name (applies to all tagged variants)
    try {
      for (const std::string &fom : table->GetSArray("event_fitter_FOM_" + full_name)) {
        fom_set.insert(fom);
      }
    } catch (DBNotFoundError &e) {
      std::string fitter_name = DS::FitResult::GetFitterNameFromFullName(full_name);
      try {
        for (const std::string &fom : table->GetSArray("event_fitter_FOM_" + fitter_name)) {
          fom_set.insert(fom);
        }
      } catch (DBNotFoundError &e) {
        info << "No FOMs found for fitter " << full_name << " or " << fitter_name
             << ". No FOM will be saved for this fitter." << newline;
      }
    }
    event_fitter_FOMs[full_name] = std::vector<std::string>(fom_set.begin(), fom_set.end());
  }
  event_classifiers = table->GetSArray("event_classifiers");
  for (const std::string &full_name : event_classifiers) {
    if (event_classifier_FOMs.find(full_name) != event_classifier_FOMs.end()) continue;
    std::set<std::string> fom_set;
    // Load FOMs declared under the base classifier name (applies to all tagged variants)
    try {
      for (const std::string &fom : table->GetSArray("event_classifier_FOM_" + full_name)) {
        fom_set.insert(fom);
      }
    } catch (DBNotFoundError &e) {
      std::string classifier_name = DS::Classifier::GetClassifierNameFromFullName(full_name);
      try {
        for (const std::string &fom : table->GetSArray("event_classifier_FOM_" + classifier_name)) {
          fom_set.insert(fom);
        }
      } catch (DBNotFoundError &e) {
        info << "No FOMs found for classifier " << full_name << " or " << classifier_name
             << ". No FOM will be saved for this classifier." << newline;
      }
    }
    event_classifier_FOMs[full_name] = std::vector<std::string>(fom_set.begin(), fom_set.end());
  }
}

void OutNtupleProc::BeginOfRun(DS::Run *run) {
  info << "Running OUTNtuple BeginOfRun" << newline;
  DBLinkPtr table = DB::Get()->GetLink("IO", "NtupleProc");
  if (options.transittime) {
    std::vector<std::string> ttime_cfg = table->GetSArray("transittime_calculator_config");
    Log::Assert(ttime_cfg.size() == 2,
                "transittime_calculator_config should have 2 entries: path calculator ID and velocity calculator ID");
    fTransitTimeCalculator = std::make_unique<RAT::TransitTimeCalculator>(ttime_cfg[0], ttime_cfg[1]);
    info << "Initialized TransitTimeCalculator with calculator ID " << ttime_cfg[0] << " and velocity calculator ID "
         << ttime_cfg[1] << newline;
  }
}

bool OutNtupleProc::OpenFile(std::string filename) {
  outputFile = TFile::Open(filename.c_str(), "RECREATE");
  // Meta Tree
  metaTree = new TTree("meta", "meta");
  MakeBranch(metaTree, "runId", &runId);
  MakeBranch(metaTree, "runType", &runType);
  MakeBranch(metaTree, "runTime", &runTime);
  MakeBranch(metaTree, "dsentries", &dsentries);
  MakeBranch(metaTree, "macro", &macro);
  MakeBranch(metaTree, "pmtType", &pmtType);
  MakeBranch(metaTree, "pmtId", &pmtId);
  MakeBranch(metaTree, "pmtChannel", &pmtChannel);
  MakeBranch(metaTree, "pmtIsOnline", &pmtIsOnline);
  MakeBranch(metaTree, "pmtCableOffset", &pmtCableOffset);
  MakeBranch(metaTree, "pmtChargeScale", &pmtChargeScale);
  MakeBranch(metaTree, "pmtPulseWidthScale", &pmtPulseWidthScale);
  MakeBranch(metaTree, "pmtX", &pmtX);
  MakeBranch(metaTree, "pmtY", &pmtY);
  MakeBranch(metaTree, "pmtZ", &pmtZ);
  MakeBranch(metaTree, "pmtU", &pmtU);
  MakeBranch(metaTree, "pmtV", &pmtV);
  MakeBranch(metaTree, "pmtW", &pmtW);
  MakeBranch(metaTree, "digitizerWindowSize", &digitizerWindowSize);
  MakeBranch(metaTree, "digitizerSampleRate_GHz", &digitizerSampleRate);
  MakeBranch(metaTree, "digitizerDynamicRange_mV", &digitizerDynamicRange);
  MakeBranch(metaTree, "digitizerResolution_mVPerADC", &digitizerVoltageResolution);
  if (options.calib) {
    MakeBranch(metaTree, "calibId", &calibId);
    MakeBranch(metaTree, "calibMode", &calibMode);
    MakeBranch(metaTree, "calibIntensity", &calibIntensity);
    MakeBranch(metaTree, "calibWavelength", &calibWavelength);
    MakeBranch(metaTree, "calibName", &calibName);
    MakeBranch(metaTree, "calibTime", &calibTime);
    MakeBranch(metaTree, "calibX", &calibX);
    MakeBranch(metaTree, "calibY", &calibY);
    MakeBranch(metaTree, "calibZ", &calibZ);
    MakeBranch(metaTree, "calibU", &calibU);
    MakeBranch(metaTree, "calibV", &calibV);
    MakeBranch(metaTree, "calibW", &calibW);
  }
  this->AssignAdditionalMetaAddresses();
  dsentries = 0;
  // Data Tree
  outputTree = new TTree("output", "output");
  // These are the *first* particles MC positions, directions, and time
  MakeBranch(outputTree, "mcpdg", &mcpdg);
  MakeBranch(outputTree, "mcx", &mcx);
  MakeBranch(outputTree, "mcy", &mcy);
  MakeBranch(outputTree, "mcz", &mcz);
  MakeBranch(outputTree, "mcu", &mcu);
  MakeBranch(outputTree, "mcv", &mcv);
  MakeBranch(outputTree, "mcw", &mcw);
  MakeBranch(outputTree, "mcke", &mcke);
  MakeBranch(outputTree, "mct", &mct);
  if (options.transittime) {
    MakeBranch(outputTree, "mcTransitTimesToPMTs", &mcTransitTimes);
  }
  // Event IDs and trigger time and nhits
  MakeBranch(outputTree, "evid", &evid);
  MakeBranch(outputTree, "subev", &subev);
  MakeBranch(outputTree, "nhits", &nhits);
  MakeBranch(outputTree, "totalcharge", &totalcharge);
  MakeBranch(outputTree, "triggerTime", &triggerTime);  // Local trigger time
  MakeBranch(outputTree, "timestamp", &timestamp);      // Global trigger time
  MakeBranch(outputTree, "trigger_word", &trigger_word);
  MakeBranch(outputTree, "triggerPeak", &triggerPeak);
  MakeBranch(outputTree, "event_cleaning_word", &event_cleaning_word);
  MakeBranch(outputTree, "timeSinceLastTrigger_us", &timeSinceLastTrigger_us);
  // MC Information
  MakeBranch(outputTree, "mcid", &mcid);
  MakeBranch(outputTree, "mcparticlecount", &mcpcount);
  MakeBranch(outputTree, "mcpecount", &mcpecount);
  MakeBranch(outputTree, "mcnhits", &mcnhits);
  MakeBranch(outputTree, "scintEdep", &scintEdep);
  MakeBranch(outputTree, "scintEdepQuenched", &scintEdepQuenched);
  // Total number of produced photons of each type
  MakeBranch(outputTree, "scintPhotons", &scintPhotons);
  MakeBranch(outputTree, "remPhotons", &remPhotons);
  MakeBranch(outputTree, "cherPhotons", &cherPhotons);
  if (options.mcparticles) {
    // Save information about *all* particles that are simulated
    // Variable naming is the same as the first particle, just plural.
    MakeBranch(outputTree, "mcpdgs", &pdgcodes);
    MakeBranch(outputTree, "mcxs", &mcPosx);
    MakeBranch(outputTree, "mcys", &mcPosy);
    MakeBranch(outputTree, "mczs", &mcPosz);
    MakeBranch(outputTree, "mcus", &mcDirx);
    MakeBranch(outputTree, "mcvs", &mcDiry);
    MakeBranch(outputTree, "mcws", &mcDirz);
    MakeBranch(outputTree, "mckes", &mcKEnergies);
    MakeBranch(outputTree, "mcts", &mcTime);
  }
  if (options.pmthits) {
    // Save full PMT hit informations
    MakeBranch(outputTree, "hitPMTID", &hitPMTID);
    // Information about *first* detected PE
    MakeBranch(outputTree, "hitPMTTime", &hitPMTTime);
    MakeBranch(outputTree, "hitPMTCharge", &hitPMTCharge);
  }
  if (options.digitizerhits) {
    // Output of the waveform analysis
    MakeBranch(outputTree, "digitNhits", &digitNhits);
    MakeBranch(outputTree, "digitPMTID", &digitPMTID);
    MakeBranch(outputTree, "digitTime", &digitTime);
    MakeBranch(outputTree, "digitCharge", &digitCharge);
    MakeBranch(outputTree, "digitNCrossings", &digitNCrossings);
    MakeBranch(outputTree, "digitNhitsCleaned", &digitHitCleanedNhits);
    MakeBranch(outputTree, "digitHitCleaningMask", &digitHitCleaningMask);
    MakeBranch(outputTree, "digitTimeOverThreshold", &digitTimeOverThreshold);
    MakeBranch(outputTree, "digitVoltageOverThreshold", &digitVoltageOverThreshold);
    MakeBranch(outputTree, "digitPeak", &digitPeak);
    MakeBranch(outputTree, "digitLocalTriggerTime", &digitLocalTriggerTime);
    MakeBranch(outputTree, "digitReconNPEs", &digitReconNPEs);
  }
  if (options.digitizerfits) {
    for (const std::string &fitter_name : waveform_fitters) {
      MakeBranch(outputTree, "fit_pmtid_" + fitter_name, &wfmFitPmtID[fitter_name]);
      MakeBranch(outputTree, "fit_time_" + fitter_name, &wfmFitTime[fitter_name]);
      MakeBranch(outputTree, "fit_charge_" + fitter_name, &wfmFitCharge[fitter_name]);
      for (const std::string &fom_name : waveform_fitter_FOMs[fitter_name]) {
        MakeBranch(outputTree, "fit_FOM_" + fitter_name + "_" + fom_name, &wfmFitFOM[fitter_name][fom_name]);
      }
    }
  }
  for (const std::string &fitter_full_name : event_fitters) {
    // fitter names are specified as either fittername__tag or just fittername.
    MakeBranch(outputTree, "x_" + fitter_full_name, &fitvalues["x_" + fitter_full_name]);
    MakeBranch(outputTree, "y_" + fitter_full_name, &fitvalues["y_" + fitter_full_name]);
    MakeBranch(outputTree, "z_" + fitter_full_name, &fitvalues["z_" + fitter_full_name]);
    MakeBranch(outputTree, "u_" + fitter_full_name, &fitvalues["u_" + fitter_full_name]);
    MakeBranch(outputTree, "v_" + fitter_full_name, &fitvalues["v_" + fitter_full_name]);
    MakeBranch(outputTree, "w_" + fitter_full_name, &fitvalues["w_" + fitter_full_name]);
    MakeBranch(outputTree, "energy_" + fitter_full_name, &fitvalues["energy_" + fitter_full_name]);
    MakeBranch(outputTree, "time_" + fitter_full_name, &fitvalues["time_" + fitter_full_name]);
    MakeBranch(outputTree, "validposition_" + fitter_full_name, &fitvalids["validposition_" + fitter_full_name]);
    MakeBranch(outputTree, "validdirection_" + fitter_full_name, &fitvalids["validdirection_" + fitter_full_name]);
    MakeBranch(outputTree, "validenergy_" + fitter_full_name, &fitvalids["validenergy_" + fitter_full_name]);
    MakeBranch(outputTree, "validtime_" + fitter_full_name, &fitvalids["validtime_" + fitter_full_name]);
    for (const std::string &fom_name : event_fitter_FOMs[fitter_full_name]) {
      MakeBranch(outputTree, fom_name + "_" + fitter_full_name, &fiteventFOMs[fitter_full_name][fom_name]);
    }
  }
  for (const std::string &classifier_full_name : event_classifiers) {
    // classifier names are specified as either classifiername__tag or just classifiername.
    for (const std::string &fom_name : event_classifier_FOMs[classifier_full_name]) {
      MakeBranch(outputTree, classifier_full_name + "_" + fom_name, &classifyeventFOMs[classifier_full_name][fom_name]);
    }
  }
  if (options.nthits) {
    MakeBranch(outputTree, "mcnNTs", &mcnNTs);
    MakeBranch(outputTree, "mcnNThits", &mcnNThits);
    MakeBranch(outputTree, "mcNTid", &mcNTid);
    MakeBranch(outputTree, "mcNThittime", &mcNThittime);
    MakeBranch(outputTree, "mcNThitx", &mcNThitx);
    MakeBranch(outputTree, "mcNThity", &mcNThity);
    MakeBranch(outputTree, "mcNThitz", &mcNThitz);
    MakeBranch(metaTree, "ntId", &ntId);
    MakeBranch(metaTree, "ntX", &ntX);
    MakeBranch(metaTree, "ntY", &ntY);
    MakeBranch(metaTree, "ntZ", &ntZ);
    MakeBranch(metaTree, "ntU", &ntU);
    MakeBranch(metaTree, "ntV", &ntV);
    MakeBranch(metaTree, "ntW", &ntW);
  }
  if (options.mchits) {
    // Save full MC PMT hit information
    MakeBranch(outputTree, "mcPMTID", &mcpmtid);
    MakeBranch(outputTree, "mcPMTNPE", &mcpmtnpe);
    MakeBranch(outputTree, "mcPMTCharge", &mcpmtcharge);

    MakeBranch(outputTree, "mcPEPMTID", &mcpepmtid);
    MakeBranch(outputTree, "mcPEHitTime", &mcpehittime);
    MakeBranch(outputTree, "mcPEFrontEndTime", &mcpefrontendtime);
    // Production process
    // 1=Cherenkov, 0=Dark noise, 2=Scint., 3=Reem., 4=Unknown
    MakeBranch(outputTree, "mcPEProcess", &mcpeprocess);
    MakeBranch(outputTree, "mcPEWavelength", &mcpewavelength);
    MakeBranch(outputTree, "mcPEx", &mcpex);
    MakeBranch(outputTree, "mcPEy", &mcpey);
    MakeBranch(outputTree, "mcPEz", &mcpez);
    MakeBranch(outputTree, "mcPECharge", &mcpecharge);
    MakeBranch(outputTree, "mcPECreationTime", &mcpecreationtime);
    MakeBranch(outputTree, "mcPECreationX", &mcpecreationx);
    MakeBranch(outputTree, "mcPECreationY", &mcpecreationy);
    MakeBranch(outputTree, "mcPECreationZ", &mcpecreationz);
    MakeBranch(outputTree, "mcPEExcitationTime", &mcpeexcitationtime);
  }
  if (options.tracking) {
    // Save particle tracking information
    MakeBranch(outputTree, "trackPDG", &trackPDG);
    MakeBranch(outputTree, "trackPosX", &trackPosX);
    MakeBranch(outputTree, "trackPosY", &trackPosY);
    MakeBranch(outputTree, "trackPosZ", &trackPosZ);
    MakeBranch(outputTree, "trackMomX", &trackMomX);
    MakeBranch(outputTree, "trackMomY", &trackMomY);
    MakeBranch(outputTree, "trackMomZ", &trackMomZ);
    MakeBranch(outputTree, "trackKE", &trackKE);
    MakeBranch(outputTree, "trackTime", &trackTime);
    MakeBranch(outputTree, "trackDep", &trackDep);
    MakeBranch(outputTree, "trackQDep", &trackQDep);
    MakeBranch(outputTree, "trackProcess", &trackProcess);
    MakeBranch(metaTree, "processCodeMap", &processCodeMap);
    MakeBranch(outputTree, "trackVolume", &trackVolume);
    MakeBranch(metaTree, "volumeCodeMap", &volumeCodeMap);
  }
  if (options.digitizerwaveforms) {
    waveformTree = new TTree("waveforms", "waveforms");
    MakeBranch(waveformTree, "evid", &evid);
    MakeBranch(waveformTree, "waveform_pmtid", &waveform_pmtid);
    MakeBranch(waveformTree, "inWindowPulseTimes", &inWindowPulseTimes);
    MakeBranch(waveformTree, "inWindowPulseCharges", &inWindowPulseCharges);
    MakeBranch(waveformTree, "waveform", &waveform);
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
  // CALIB Branches
  if (options.calib && (!done_writing_calib)) {
    DS::Calib *calib = ds->GetCalib();
    calibId = calib->GetID();
    calibMode = calib->GetMode();
    calibIntensity = calib->GetIntensity();
    calibWavelength = calib->GetWavelength();
    calibName = calib->GetSourceName();
    calibTime = TTimeStamp_to_UnixTime(calib->GetUTC());
    TVector3 calib_pos = calib->GetPosition();
    calibX = calib_pos.X();
    calibY = calib_pos.Y();
    calibZ = calib_pos.Z();
    TVector3 calib_dir = calib->GetDirection();
    calibU = calib_dir.X();
    calibV = calib_dir.Y();
    calibW = calib_dir.Z();
    done_writing_calib = true;
  }
  runBranch = DS::RunStore::GetRun(ds);
  DS::PMTInfo *pmtinfo = runBranch->GetPMTInfo();
  DS::NestedTubeInfo *ntinfo = runBranch->GetNestedTubeInfo();
  const DS::ChannelStatus *channel_status = runBranch->GetChannelStatus();
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

  // Transit time to PMTs
  if (options.transittime) {
    TVector3 event_vertex(mcx, mcy, mcz);
    mcTransitTimes.clear();
    for (int id = 0; id < pmtinfo->GetPMTCount(); id++) {
      TVector3 pmt_pos = pmtinfo->GetPosition(id);
      TransitTimeCalculator::Result ttime_result = fTransitTimeCalculator->Compute(event_vertex, pmt_pos);
      mcTransitTimes.push_back(ttime_result.totalTime);
    }
  }

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
    trackDep.clear();
    trackQDep.clear();
    trackProcess.clear();
    trackVolume.clear();

    std::vector<double> xtrack, ytrack, ztrack;
    std::vector<double> pxtrack, pytrack, pztrack;
    std::vector<double> kinetic, globaltime, deposited, quenchedDeposited;
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
      deposited.clear();
      quenchedDeposited.clear();
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
        deposited.push_back(step->GetDepositedEnergy());
        quenchedDeposited.push_back(step->GetQuenchedDepositedEnergy());
        xtrack.push_back(tv.X());
        ytrack.push_back(tv.Y());
        ztrack.push_back(tv.Z());
        pxtrack.push_back(momentum.X());
        pytrack.push_back(momentum.Y());
        pztrack.push_back(momentum.Z());
      }
      trackKE.push_back(kinetic);
      trackTime.push_back(globaltime);
      trackDep.push_back(deposited);
      trackQDep.push_back(quenchedDeposited);
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
  mcpepmtid.clear();
  mcpehittime.clear();
  mcpefrontendtime.clear();
  mcpeprocess.clear();
  mcpewavelength.clear();
  mcpex.clear();
  mcpey.clear();
  mcpez.clear();
  mcpecharge.clear();
  mcpecreationtime.clear();
  mcpecreationx.clear();
  mcpecreationy.clear();
  mcpecreationz.clear();
  mcpeexcitationtime.clear();

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
        mcpepmtid.push_back(mcpmt->GetID());
        mcpehittime.push_back(mcph->GetHitTime());
        mcpefrontendtime.push_back(mcph->GetFrontEndTime());
        mcpewavelength.push_back(mcph->GetLambda());
        mcpex.push_back(position.X());
        mcpey.push_back(position.Y());
        mcpez.push_back(position.Z());
        mcpecharge.push_back(mcph->GetCharge());
        mcpecreationtime.push_back(mcph->GetCreationTime());
        TVector3 creationPos = mcph->GetCreationPosition();
        mcpecreationx.push_back(creationPos.X());
        mcpecreationy.push_back(creationPos.Y());
        mcpecreationz.push_back(creationPos.Z());
        mcpeexcitationtime.push_back(mcph->GetExcitationTime());
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
  if (options.nthits) {
    mcnNTs = mc->GetMCNestedTubeCount();
    mcnNThits = 0;
    for (int iNT = 0; iNT < mc->GetMCNestedTubeCount(); iNT++) {
      DS::MCNestedTube *mcnt = mc->GetMCNestedTube(iNT);
      mcnNThits += mcnt->GetMCNestedTubeHitCount();
      mcNTid.push_back(mcnt->GetID());
      G4ThreeVector position = ntinfo->GetPosition(mcnt->GetID());
      for (int ih = 0; ih < mcnt->GetMCNestedTubeHitCount(); ih++) {
        RAT::DS::MCNestedTubeHit *mcph = mcnt->GetMCNestedTubeHit(ih);
        mcNThittime.push_back(mcph->GetHitTime());
        mcNThitx.push_back(position.x());
        mcNThity.push_back(position.y());
        mcNThitz.push_back(position.z());
      }
    }
  }

  // EV Branches
  for (subev = 0; subev < ds->GetEVCount(); subev++) {
    DS::EV *ev = ds->GetEV(subev);
    evid = ev->GetID();
    triggerTime = ev->GetCalibratedTriggerTime();
    timestamp = TTimeStamp_to_UnixTime(ev->GetUTC()) - TTimeStamp_to_UnixTime(runBranch->GetStartTime()) + triggerTime;
    trigger_word = ev->GetTriggerWord();
    triggerPeak = ev->GetTriggerPeak();
    event_cleaning_word = ev->GetEventCleaningWord();
    timeSinceLastTrigger_us = ev->GetDeltaT() / 1000.;

    for (DS::FitResult *fit : ev->GetFitResults()) {
      std::string full_name = fit->GetFullName();
      // Check the validity and write it out
      if (std::find(event_fitters.begin(), event_fitters.end(), full_name) == event_fitters.end()) {
        info << "Fitter " << full_name
             << " not found in the list of requested fitters. Will not be written to the ntuple." << newline;
        continue;
      }
      if (fit->GetEnablePosition()) {
        TVector3 pos = fit->GetPosition();
        fitvalues.at("x_" + full_name) = pos.X();
        fitvalues.at("y_" + full_name) = pos.Y();
        fitvalues.at("z_" + full_name) = pos.Z();
        fitvalids.at("validposition_" + full_name) = fit->GetValidPosition();
      }
      if (fit->GetEnableDirection()) {
        TVector3 dir = fit->GetDirection();
        fitvalues.at("u_" + full_name) = dir.X();
        fitvalues.at("v_" + full_name) = dir.Y();
        fitvalues.at("w_" + full_name) = dir.Z();
        fitvalids.at("validdirection_" + full_name) = fit->GetValidDirection();
      }
      if (fit->GetEnableEnergy()) {
        fitvalues.at("energy_" + full_name) = fit->GetEnergy();
        fitvalids.at("validenergy_" + full_name) = fit->GetValidEnergy();
      }
      if (fit->GetEnableTime()) {
        fitvalues.at("time_" + full_name) = fit->GetTime();
        fitvalids.at("validtime_" + full_name) = fit->GetValidTime();
      }
      for (const std::string &fom_name : event_fitter_FOMs[full_name]) {
        fiteventFOMs[full_name][fom_name] = fit->GetFigureOfMerit(fom_name);
      }
    }

    for (DS::Classifier *clf : ev->GetClassifierResults()) {
      std::string full_name = clf->GetFullName();
      // Check the validity and write it out
      if (std::find(event_classifiers.begin(), event_classifiers.end(), full_name) == event_classifiers.end()) {
        info << "Classifier " << full_name
             << " not found in the list of requested classifiers. Will not be written to the ntuple." << newline;
        continue;
      }
      for (const std::string &fom_name : event_classifier_FOMs[full_name]) {
        classifyeventFOMs[full_name][fom_name] = clf->GetClassificationResult(fom_name);
      }
    }

    nhits = ev->GetPMTCount();
    totalcharge = ev->GetTotalCharge();
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
      digitTimeOverThreshold.clear();
      digitVoltageOverThreshold.clear();
      digitPeak.clear();
      digitPMTID.clear();
      digitLocalTriggerTime.clear();
      digitReconNPEs.clear();
      digitHitCleanedNhits = 0;
      digitHitCleaningMask.clear();

      if (options.digitizerfits) {
        for (const std::string &fitter_name : waveform_fitters) {
          // construct arrays for all fitters
          wfmFitPmtID[fitter_name].clear();
          wfmFitTime[fitter_name].clear();
          wfmFitCharge[fitter_name].clear();
          for (const std::string &fom_name : waveform_fitter_FOMs[fitter_name]) {
            wfmFitFOM[fitter_name][fom_name].clear();
          }
        }
      }

      for (int pmtc : ev->GetAllDigitPMTIDs()) {
        RAT::DS::DigitPMT *digitpmt = ev->GetOrCreateDigitPMT(pmtc);
        digitPMTID.push_back(digitpmt->GetID());
        digitTime.push_back(digitpmt->GetDigitizedTime());
        digitCharge.push_back(digitpmt->GetDigitizedCharge());
        digitNCrossings.push_back(digitpmt->GetNCrossings());
        digitTimeOverThreshold.push_back(digitpmt->GetTimeOverThreshold());
        digitReconNPEs.push_back(digitpmt->GetReconNPEs());
        digitHitCleaningMask.push_back(digitpmt->GetHitCleaningMask());
        digitVoltageOverThreshold.push_back(digitpmt->GetVoltageOverThreshold());
        digitPeak.push_back(digitpmt->GetPeakVoltage());
        digitLocalTriggerTime.push_back(digitpmt->GetLocalTriggerTime());
        if (options.digitizerfits) {
          const std::vector<std::string> fitters = digitpmt->GetFitterNames();
          for (std::string fitter_name : fitters) {
            DS::WaveformAnalysisResult *fit_result = digitpmt->GetOrCreateWaveformAnalysisResult(fitter_name);
            for (int hitidx = 0; hitidx < fit_result->getNPEs(); hitidx++) {
              wfmFitPmtID[fitter_name].push_back(digitpmt->GetID());
              wfmFitTime[fitter_name].push_back(fit_result->getTime(hitidx));
              wfmFitCharge[fitter_name].push_back(fit_result->getCharge(hitidx));
              for (const std::string &fom_name : waveform_fitter_FOMs[fitter_name]) {
                wfmFitFOM[fitter_name][fom_name].push_back(fit_result->getFOM(fom_name, hitidx));
              }
            }
          }
        }
      }
      digitNhits = ev->DigitNhits();
      digitHitCleanedNhits = ev->DigitNhitsCleaned();
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
    totalcharge = 0;
    triggerTime = 0;
    triggerPeak = 0;
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
      digitVoltageOverThreshold.clear();
      digitTimeOverThreshold.clear();
      digitPeak.clear();
      digitPMTID.clear();
      digitLocalTriggerTime.clear();
      digitReconNPEs.clear();
      digitHitCleanedNhits = 0;
      digitHitCleaningMask.clear();
      if (options.digitizerfits) {
        for (const std::string &fitter_name : waveform_fitters) {
          // construct arrays for all fitters
          wfmFitPmtID[fitter_name].clear();
          wfmFitTime[fitter_name].clear();
          wfmFitCharge[fitter_name].clear();
          for (const std::string &fom_name : waveform_fitter_FOMs[fitter_name]) {
            wfmFitFOM[fitter_name][fom_name].clear();
          }
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

void OutNtupleProc::EndOfRun(DS::Run *run) {
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
      pmtChargeScale.push_back(ch_status->GetChargeScaleByPMTID(id));
      pmtPulseWidthScale.push_back(ch_status->GetPulseWidthScaleByPMTID(id));
      pmtX.push_back(position.X());
      pmtY.push_back(position.Y());
      pmtZ.push_back(position.Z());
      pmtU.push_back(direction.X());
      pmtV.push_back(direction.Y());
      pmtW.push_back(direction.Z());
    }
    if (options.nthits) {
      DS::NestedTubeInfo *ntinfo = runBranch->GetNestedTubeInfo();
      for (int id = 0; id < ntinfo->GetNestedTubeCount(); id++) {
        G4ThreeVector position = ntinfo->GetPosition(id);
        G4ThreeVector direction = ntinfo->GetDirection(id);
        ntId.push_back(id);
        ntX.push_back(position.x());
        ntY.push_back(position.y());
        ntZ.push_back(position.z());
        ntU.push_back(direction.x());
        ntV.push_back(direction.y());
        ntW.push_back(direction.z());
      }
    }
    runId = runBranch->GetID();
    runType = runBranch->GetType();
    // Converting to unix time
    TTimeStamp rootTime = runBranch->GetStartTime();
    runTime = TTimeStamp_to_UnixTime(rootTime.GetSec());
    macro = Log::GetMacro();
    FillMeta();
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
    outputFile = nullptr;
  }
}

void OutNtupleProc::SetS(std::string param, std::string value) {
  if (param == "file") {
    this->defaultFilename = value;
  } else if (param == "prune") {
    std::stringstream ss(value);
    std::string branch_name;
    while (std::getline(ss, branch_name, ',')) {
      prunedBranches.push_back(branch_name);
    }
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void OutNtupleProc::SetI(std::string param, int value) {
  if (param == "include_tracking") {
    options.tracking = value ? true : false;
  } else if (param == "include_mcparticles") {
    options.mcparticles = value ? true : false;
  } else if (param == "include_pmthits") {
    options.pmthits = value ? true : false;
  } else if (param == "include_nestedtubehits") {
    options.nthits = value ? true : false;
  } else if (param == "include_untriggered_events") {
    options.untriggered = value ? true : false;
  } else if (param == "include_mchits") {
    options.mchits = value ? true : false;
  } else if (param == "include_digitizerwaveforms") {
    options.digitizerwaveforms = value ? true : false;
  } else if (param == "include_digitizerhits") {
    options.digitizerhits = value ? true : false;
  } else if (param == "include_digitizerfits") {
    options.digitizerfits = value ? true : false;
  } else {
    throw Processor::ParamUnknown(param);
  }
}

}  // namespace RAT
