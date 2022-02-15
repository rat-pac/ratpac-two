/* SplitEVDAQ is an extension of the SimpleDAQ that converts hits into multiple
 * triggers, and places those in new events (not subevents) to better simulate
 * data and produce combined datasets.
*/
#include <vector>
#include <algorithm>
#include <RAT/SplitEVDAQProc.hh>
#include <RAT/DB.hh>
#include <G4ThreeVector.hh>
#include <RAT/DetectorConstruction.hh>

using namespace std;

namespace RAT {

SplitEVDAQProc::SplitEVDAQProc() : Processor("splitevdaq") {
  ldaq               = DB::Get()->GetLink("SplitEVDAQ");
  fEventCounter      = 0;
  fPulseWidth        = ldaq->GetD("pulse_width");
  fTriggerThreshold  = ldaq->GetD("trigger_threshold");
  fTriggerWindow     = ldaq->GetD("trigger_window");
  fPmtLockout        = ldaq->GetD("pmt_lockout");
  fTriggerLockout    = ldaq->GetD("trigger_lockout");
  fTriggerResolution = ldaq->GetD("trigger_resolution");
  fLookback          = ldaq->GetD("lookback");
  fMaxHitTime        = ldaq->GetD("max_hit_time");
  fDiscriminator     = ldaq->GetD("discriminator");
  fPmtType           = ldaq->GetI("pmt_type");
  fTriggerOnNoise    = ldaq->GetI("trigger_on_noise");
}

Processor::Result SplitEVDAQProc::DSEvent(DS::Root *ds) {
  // This DAQ will convert hits on PMTs through time into trigger pulses
  // which can fire a global event trigger. Each trigger will correspond
  // to a single sub-event in the datastructure. 
  // Not included yet
  // - Noise on the trigger pulse height, rise-time, etc
  // - Disciminator on charge (all hits assumed to trigger)
  // Note on the discriminator mk-I. Right now each photon is presumed to
  // deposit its charge instantaneously and independently, which is faster
  // for simulations but is less accurate for events with multi-pe since
  // we aren't summing the hits to create a PMT waveform. We should add this
  // feature as an option to test the accuracy of this simplified model, noting
  // that the feature will likely cause a slowdown when used because of its
  // increased complexity.

  DS::MC *mc = ds->GetMC();
  // Prune the previous EV branchs if one exists
  if( ds->ExistEV() ) ds->PruneEV();

  // First loop through the PMTs and create a summed trigger
  vector<double> trigPulses;
  for (int imcpmt=0; imcpmt < mc->GetMCPMTCount(); imcpmt++)
  {
    DS::MCPMT *mcpmt = mc->GetMCPMT(imcpmt);
    mcpmt->SortMCPhotons();
    if( mcpmt->GetType() != fPmtType ) continue;
    double lastTrigger = -100000.0;
    for(int pidx=0; pidx < mcpmt->GetMCPhotonCount(); pidx++)
    {
      DS::MCPhoton* photon = mcpmt->GetMCPhoton(pidx);
      // Do we want to trigger on noise hits?
      if( !fTriggerOnNoise && photon->IsDarkHit() ) continue;
      double time = photon->GetFrontEndTime();
      double charge = photon->GetCharge();
      if (time > fMaxHitTime) continue;
      // Pass discriminator threshold?
      if (charge < fDiscriminator) continue;
      if (time > (lastTrigger + fPmtLockout))
      {
        trigPulses.push_back(time);
        lastTrigger = time;
      }
    }
  }
  if( trigPulses.size() < 1 ) return Processor::OK; // We're done, no triggers

  double start = *std::min_element(trigPulses.begin(), trigPulses.end());
  start = floor(start / fTriggerResolution) * fTriggerResolution;
  double end = *std::max_element(trigPulses.begin(), trigPulses.end());
  end = (floor(end / fTriggerResolution)+1) * fTriggerResolution;
  std::sort(trigPulses.begin(), trigPulses.end());

  // Turns hits into a histogram of trigger pulse leading edges
  //        _
  //   _   | |    _
  // _| |__| |___| |___
  int nbins = floor( (end - start) / fTriggerResolution )+1;
  double bw = fTriggerResolution;
  vector<double> triggerTrain( nbins );
  for( auto v : trigPulses )
  {
    int select = int( (v - start) / bw );
    triggerTrain[select] += 1.0;
  }

  // Spread each bin out to the trigger pulse width to pass to a discriminator
  //             | 
  //.............|_____ .... trigger threshold .....
  //        _____|     |____
  //       |     |          |____
  //   ____|     |               |____
  // _|          | global trigger!    |___
  vector<double> triggerHistogram( nbins );
  for(int i=0; i<nbins; i++)
  {
    double x = triggerTrain[i];
    if( x > 0 )
    {
      for( int j=i; j<i+fPulseWidth; j++ )
      {
        if( j >= nbins )
          break;
        triggerHistogram[j] += x;
      }
    }
  }

  // Trigger the detector based on fTriggerThreshold
  double lastTrigger = -100000.0;
  vector<double> triggerTimes;
  for(int i=0; i<nbins; i++)
  {
    double v = triggerHistogram[i];
    if( v >= fTriggerThreshold ) // check for trigger
    {
      if( (i*bw)+start > (lastTrigger + fTriggerWindow + fTriggerLockout) )
      {
        lastTrigger = (i*bw)+start;
        triggerTimes.push_back(lastTrigger);
      }
    }
  }

  // Place the correct hits, charges, etc into the right trigger windows
  lastTrigger = 0;
  for( auto tt : triggerTimes )
  {
    DS::EV *ev = ds->AddNewEV();
    ev->SetID(fEventCounter++);
    ev->SetCalibratedTriggerTime( tt );
    ev->SetDeltaT( tt - lastTrigger );
    lastTrigger = tt;
    double totalEVCharge = 0; // What does total charge get used for?
    for( int imcpmt=0; imcpmt < mc->GetMCPMTCount(); imcpmt++)
    {
      DS::MCPMT *mcpmt = mc->GetMCPMT(imcpmt);
      int pmtID = mcpmt->GetID();
      // Check if the mcpmt has a time within one pulsewidth of the trigger window
      bool pmtInEvent = false;
      double integratedCharge = 0;
      vector<double> hitTimes;
      if( mcpmt->GetMCPhotonCount() > 0 )
      {
        for(int pidx=0; pidx < mcpmt->GetMCPhotonCount(); pidx++)
        {
          DS::MCPhoton* photon = mcpmt->GetMCPhoton(pidx);
          double time = photon->GetFrontEndTime();
          if ( ( time > (tt - fLookback) ) && ( time < (tt + fTriggerWindow) ) )
          {
            pmtInEvent = true;
            hitTimes.push_back(time);
            integratedCharge += photon->GetCharge();
          }
        }
      }
      // If the integrated charge in the trigger window is below discriminator
      // threshold do not record the event.
      if ( integratedCharge < fDiscriminator )
        pmtInEvent = false;
      std::sort(hitTimes.begin(), hitTimes.end());
      if( pmtInEvent )
      {
        DS::PMT* pmt = ev->AddNewPMT();
        pmt->SetID(pmtID);
        double true_hit_time = hitTimes[0];
        // PMT Hit time relative to the trigger
        pmt->SetTime( true_hit_time - tt );
        pmt->SetCharge( integratedCharge );
        if( mcpmt->GetType() == fPmtType ) totalEVCharge += integratedCharge;
      }
    } // Done looping over PMTs
    ev->SetTotalCharge( totalEVCharge );
  }
  
  return Processor::OK;
}

void SplitEVDAQProc::SetD(std::string param, double value)
{
  if( param == "pulse_width" )
    fPulseWidth = value;
  else if( param == "trigger_threshold" )
    fTriggerThreshold = value;
  else if( param == "trigger_window" )
    fTriggerWindow = value;
  else if( param == "pmt_lockout" )
    fPmtLockout = value;
  else if( param == "trigger_lockout" )
    fTriggerLockout = value;
  else if( param == "trigger_resolution" )
    fTriggerResolution = value;
  else if( param == "lookback" )
    fLookback = value;
  else if( param == "max_hit_time" )
    fMaxHitTime = value;
  else if( param == "discriminator" )
    fDiscriminator = value;
  else
    throw ParamUnknown(param);
}

void SplitEVDAQProc::SetI(std::string param, int value)
{
  if( param == "pulse_type" )
    fPmtType = value;
  else if( param == "trigger_on_noise" )
    fTriggerOnNoise = value;
  else
    throw ParamUnknown(param);
}

} // namespace RAT

