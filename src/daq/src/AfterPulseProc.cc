
/*
After-pulsing adapted from snoplus rat. Settings for after-pulsing are controlled using AFTERPULSEProc.ratdb. Parameters
are as follows:

        "daq": describes which index of DAQ.ratdb to pull the trigger_window value from.
        "afterpulse_flag": determines where to pull the timing distribution and after-pulse fraction from
                        "0": use default timing distribution and after-pulse fraction
                        "1": use per model timing dist and ap fraction
                        "2": use per model timing dis and per individual ap fraction
        "afterpulse_fraction": corresponds to the fraction of events for which an after-pulse will occur
        "apTime": the time values use in the creation of the after pulse
        "apProb": the CDF corresponding to the values in apTime used to determine the placement of an ap
        "PMTTime": the time values for the transit time of the after-pulse
        "PMTProb": the PDF corresponding to the values in PMTTime used to determine the transit time
        "CableDelay": an additional flat value which gets included in the front end time
*/

#include <TTimeStamp.h>

#include <RAT/AfterPulseProc.hh>
#include <RAT/DB.hh>
#include <RAT/DS/RunStore.hh>
#include <RAT/DetectorConstruction.hh>
#include <RAT/Log.hh>
#include <RAT/MiniCleanPMTCharge.hh>
#include <Randomize.hh>
#include <algorithm>
#include <map>
#include <random>
#include <vector>

namespace RAT {

AfterPulseProc::AfterPulseProc() : Processor("afterpulse") {
  DBLinkPtr lafterpulse = DB::Get()->GetLink("AFTERPULSEPROC");
  fDefAPFraction = lafterpulse->GetD("afterpulse_fraction");
  fAPFlag = lafterpulse->GetI("afterpulse_flag");
  fDefAPTime = lafterpulse->GetDArray("apTime");
  fDefAPProb = lafterpulse->GetDArray("apProb");
  fDefPMTTime = lafterpulse->GetDArray("PMTTime");
  fDefPMTProb = lafterpulse->GetDArray("PMTProb");
  fDefCableDelay = lafterpulse->GetD("CableDelay");
  fDAQ = lafterpulse->GetS("daq");
}

Processor::Result AfterPulseProc::DSEvent(DS::Root* ds) {
  // After pulse moved to a processor from GSim, this is a special
  // case of a processor modifying the MC branch
  // Write over MC
  DS::MC* mc = ds->GetMC();
  DS::Run* run = DS::RunStore::Get()->GetRun(ds);
  DS::PMTInfo* pmtinfo = run->GetPMTInfo();

  TTimeStamp timestamp = mc->GetUTC();
  double timesec = timestamp.GetSec();
  double timensec = timestamp.GetNanoSec();  // might need to add half a ns
  uint64_t event_time = static_cast<uint64_t>(timensec + timesec * 1e9);

  std::map<int, int> mcpmtObjects;
  std::vector<double> realHitTimes;

  fPMTCount = mc->GetMCPMTCount();
  fTriggerWindow = DB::Get()->GetLink(fDAQ)->GetD("trigger_window");
  fAfterPulseTime.resize(fPMTCount);
  fFrontEndTime.resize(fPMTCount);
  countAP.resize(fPMTCount, 0);
  GenerateAfterPulse(mc, pmtinfo, event_time);

  return Processor::OK;
}

void AfterPulseProc::GenerateAfterPulse(DS::MC* mc, DS::PMTInfo* pmtinfo, const uint64_t event_time) {
  for (int ipmt = 0; ipmt < fPMTCount; ipmt++) {
    const std::string modelName = pmtinfo->GetModelName(mc->GetMCPMT(ipmt)->GetType());
    double APFraction;
    double CableDelay;

    std::vector<double> APTime;
    std::vector<double> APProb;
    std::vector<double> PMTTime;
    std::vector<double> PMTProb;
    // switch statement for pulling values from, will use the default values in the case of no flag parameter
    switch (fAPFlag) {
      case 0: {
        APFraction = fDefAPFraction;
        APTime = fDefAPTime;
        APProb = fDefAPProb;
        PMTTime = fDefPMTTime;
        PMTProb = fDefPMTProb;
        CableDelay = fDefCableDelay;
        break;
      }
      case 1: {
        try {
          DBLinkPtr lmodel = DB::Get()->GetLink("AFTERPULSEPROC", modelName);
          APFraction = lmodel->GetD("afterpulse_fraction");
          APTime = lmodel->GetDArray("apTime");
          APProb = lmodel->GetDArray("apProb");
          PMTTime = lmodel->GetDArray("PMTTime");
          PMTProb = lmodel->GetDArray("PMTProb");
          CableDelay = lmodel->GetD("CableDelay");
        } catch (DBNotFoundError& e) {
          std::cout << "Per model information missing from AFTERPULSEProc.ratdb, using default values storedin "
                       "AFTERPULSEProc.ratdb."
                    << "\n";
          APFraction = fDefAPFraction;
          APTime = fDefAPTime;
          APProb = fDefAPProb;
          PMTTime = fDefPMTTime;
          PMTProb = fDefPMTProb;
          CableDelay = fDefCableDelay;
        }
        break;
      }
      case 2: {
        try {
          APFraction = pmtinfo->GetAfterPulseFraction(ipmt);
          DBLinkPtr modlmodel = DB::Get()->GetLink("AFTERPULSEPROC", modelName);
          APTime = modlmodel->GetDArray("apTime");
          APProb = modlmodel->GetDArray("apProb");
          PMTTime = modlmodel->GetDArray("PMTTime");
          PMTProb = modlmodel->GetDArray("PMTProb");
          CableDelay = modlmodel->GetD("CableDelay");
        } catch (DBNotFoundError& e) {
          std::cout << "Per individual PMT information missing from PMTINFO.ratdb, using default values storedin "
                       "AFTERPULSEProc.ratdb."
                    << "\n";
          APFraction = fDefAPFraction;
          APTime = fDefAPTime;
          APProb = fDefAPProb;
          PMTTime = fDefPMTTime;
          PMTProb = fDefPMTProb;
          CableDelay = fDefCableDelay;
        }
        break;
      }
      default: {
        APFraction = fDefAPFraction;
        APTime = fDefAPTime;
        APProb = fDefAPProb;
        PMTTime = fDefPMTTime;
        PMTProb = fDefPMTProb;
        CableDelay = fDefCableDelay;
      }
    }

    int npe = mc->GetMCPMT(ipmt)->GetMCPhotonCount();
    for (int ipe = 0; ipe < npe; ipe++) {
      double rand = G4UniformRand();
      if (mc->GetMCPMT(ipmt)->GetMCPhoton(ipe)->IsDarkHit() == false) {
        uint64_t frontEndTime = mc->GetMCPMT(ipmt)->GetMCPhoton(ipe)->GetFrontEndTime();
        fFrontEndTime[ipmt].push_back(frontEndTime);
        if (rand <= APFraction) {
          uint64_t afterPulseTime = CalculateAfterPulseTime(APFraction, APTime, APProb);
          uint64_t globalAPTime = event_time + frontEndTime + afterPulseTime;
          fAfterPulseTime[ipmt].push_back(globalAPTime);
          countAP[ipmt] += 1;
        }
      }
    }
    DetectAfterPulse(mc, pmtinfo, event_time, PMTTime, PMTProb, CableDelay);
  }
}

double AfterPulseProc::CalculateAfterPulseTime(double APFraction, std::vector<double> APTime,
                                               std::vector<double> APProb) {
  const double randtime = G4UniformRand();
  size_t up = 1;
  for (int i = 1; i < APTime.size(); i++) {
    if (randtime < APProb[i]) {
      up = i;
      i = APTime.size();
    }
  }
  double newtime = (randtime - APProb[(up - 1)]) * (APTime[up] - APTime[(up - 1)]) / (APProb[up] - APProb[(up - 1)]) +
                   APTime[(up - 1)];
  return newtime;
}

void AfterPulseProc::SetI(std::string param, int value) {
  if (param == "flag") {
    fAPFlag = value;
  } else {
    throw ParamUnknown(param);
  }
}

void AfterPulseProc::SetD(std::string param, double value) {
  if (param == "fraction") {
    fDefAPFraction = value;
  } else {
    throw ParamUnknown(param);
  }
}

double AfterPulseProc::PickFrontEnd(double time, std::vector<double> PMTTime, std::vector<double> PMTProb,
                                    double CableDelay) {  // function for picking the front end time

  if (PMTTime.size() != PMTProb.size()) {
    Log::Die("PDFPMTTime: time and probability arrays of different length");
  }
  if (PMTTime.size() < 2) {
    Log::Die("PDFPMTTime: cannot define a PDF with fewer than 2 points");
  }

  double integral = 0.0;
  std::vector<double> PMTProbCumu(PMTTime.size());
  PMTProbCumu[0] = 0;
  for (size_t i = 0; i < PMTTime.size() - 1; i++) {
    integral += (PMTTime[i + 1] - PMTTime[i]) * (PMTProb[i] + PMTProb[i + 1]) / 2.0;
    PMTProbCumu[i + 1] = integral;
  }
  for (size_t i = 0; i < PMTTime.size(); i++) {
    PMTProb[i] /= integral;
    PMTProbCumu[i] /= integral;
  }

  double rval = G4UniformRand();
  for (size_t i = 1; i < PMTTime.size(); i++) {
    if (rval <= PMTProbCumu[i]) {
      return time + CableDelay +
             (rval - PMTProbCumu[i - 1]) * (PMTTime[i] - PMTTime[i - 1]) / (PMTProbCumu[i] - PMTProbCumu[i - 1]) +
             PMTTime[i - 1];
    }
  }
  info << "PDFPMTTime::PickTime: impossible condition encountered - returning highest defined time" << endl;
  return time + CableDelay + PMTTime[PMTTime.size() - 1];
}

void AfterPulseProc::DetectAfterPulse(DS::MC* mc, DS::PMTInfo* pmtinfo, const uint64_t event_time,
                                      std::vector<double> PMTTime, std::vector<double> PMTProb, double CableDelay) {
  for (int iPMT = 0; iPMT < fPMTCount; iPMT++) {
    int npe = fFrontEndTime[iPMT].size();
    for (int iHit = 0; iHit < npe; iHit++) {           // Loop over PEs
      for (int iAP = 0; iAP < countAP[iPMT]; iAP++) {  // Loops over APs
        uint64_t front_end_time = fFrontEndTime[iPMT][iHit];
        uint64_t after_pulse_time = fAfterPulseTime[iPMT][iAP];  // time of after-pulse
        uint64_t low_window = event_time + front_end_time - static_cast<uint64_t>(2 * fTriggerWindow);
        uint64_t high_window = event_time + front_end_time + static_cast<uint64_t>(2 * fTriggerWindow);

        // If the after-pulse falls near an event window, create a PE,
        // flagged as an after-pulse. Record the time, trigger processors
        // determine whether this after-pulse actually falls into an event.

        if (after_pulse_time > low_window && after_pulse_time < high_window) {
          int64_t time = after_pulse_time - event_time;
          DS::MCPhoton* photoelectron = mc->GetMCPMT(iPMT)->AddNewMCPhoton();

          photoelectron->SetHitTime(time);
          photoelectron->SetCharge(1);
          photoelectron->SetAfterPulse(true);
          photoelectron->SetDarkHit(false);

          double ap_front_end_time = PickFrontEnd(time, PMTTime, PMTProb, CableDelay);
          photoelectron->SetFrontEndTime(ap_front_end_time);
          // Don't double count, erase the PE from after-pulsing vector
          fAfterPulseTime[iPMT].erase(fAfterPulseTime[iPMT].begin() + iAP);
          countAP[iPMT] -= 1;
        }

        // If an event comes along well past the time of the after-pulse
        // clear the memory of the after-pulse and corresponding front-end-time.
        if (event_time + front_end_time > after_pulse_time + high_window) {
          if (iHit == npe - 1) {  // only remove once
            fAfterPulseTime[iPMT].erase(fAfterPulseTime[iPMT].begin() + iAP);
            countAP[iPMT] -= 1;  // keep track of the fact a PE was removed
          }
          if (iAP == countAP[iPMT] - 1) {  // only remove once
            fFrontEndTime[iPMT].erase(fFrontEndTime[iPMT].begin() + iHit);
          }
        }
      }  // end loop over APs
    }    // end loop over PEs
  }      // end loop over PMTs
}
}  // namespace RAT
