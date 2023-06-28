#include <G4ThreeVector.hh>
#include <RAT/DB.hh>
#include <RAT/DetectorConstruction.hh>
#include <RAT/LessSimpleDAQProc.hh>
#include <RAT/Log.hh>
#include <iostream>
#include <vector>

#include "RAT/Log.hh"

namespace RAT {

LessSimpleDAQProc::LessSimpleDAQProc() : Processor("lesssimpledaq") {
  // DBLinkPtr ldaq = DB::Get()->GetLink("DAQ");
  // fSPECharge = ldaq->GetDArray("SPE_charge"); // convert pC to
  // gain-normalized units
  fEventCounter = 0;
}

Processor::Result LessSimpleDAQProc::DSEvent(DS::Root *ds) {
  // This simple simulation assumes only tubes hit by a photon register
  // a hit, and that every MC event corresponds to one triggered event
  // The time of the PMT hit is that of the first photon.

  DS::MC *mc = ds->GetMC();
  if (ds->ExistEV()) {  // there is already a EV branch present
    ds->PruneEV();      // remove it, otherwise we'll have multiple detector events
    // in this physics event ** we really should warn the user what is taking
    // place
  }

  double totalQ = 0.0;
  double time, timeTmp;
  int nSubEvents = 0;
  int oldGroup;
  double postTriggerWindow = 600.;  // ns
  double preTriggerWindow = -200.;  // ns
  double triggerWindow = 200.;      // ns
  unsigned long triggerThreshold = 6;
  unsigned long hits = 0;

  // info <<"New Event====================================" << newline;
  //  First part is to load into std::vector PMT information for full event
  std::vector<double> timeAndChargeAndID;
  std::vector<std::vector<double>> pmtARRAY;
  // Place the time and charge of a PMT into an matrix
  for (int imcpmt = 0; imcpmt < mc->GetMCPMTCount(); imcpmt++) {
    DS::MCPMT *mcpmt = mc->GetMCPMT(imcpmt);
    if (mcpmt->GetMCPhotonCount() > 0) {
      for (int i = 0; i < mcpmt->GetMCPhotonCount(); i++) {
        timeAndChargeAndID.push_back(mcpmt->GetMCPhoton(i)->GetFrontEndTime());
        timeAndChargeAndID.push_back(mcpmt->GetMCPhoton(i)->GetCharge());
        timeAndChargeAndID.push_back(mcpmt->GetID());
        timeAndChargeAndID.push_back(i);
        timeAndChargeAndID.push_back(mcpmt->GetMCPhotonCount());
        timeAndChargeAndID.push_back(mcpmt->GetMCPhoton(i)->GetHitTime());
        timeAndChargeAndID.push_back(mcpmt->GetMCPhoton(i)->IsDarkHit());

        pmtARRAY.push_back(timeAndChargeAndID);
        timeAndChargeAndID.resize(0);
      }
    }
  }
  sort(pmtARRAY.begin(),
       pmtARRAY.end());  // pmt hits sorted as a function of time

  // Second part is to find cluster times. This is important for IBD/ neutron
  // capture
  std::vector<Double_t> clusterTime;
  // Give an unrealistic time to compare to
  clusterTime.push_back(1000000000000000000);

  // get the number odd subevent
  // and tally the cluster time of each subevent
  // info <<"oooooooooooooooo sorted oooooooooooo" << newline;

  for (unsigned long pmtIndex = 0; pmtIndex < pmtARRAY.size(); pmtIndex++) {
    time = pmtARRAY[pmtIndex][0];
    oldGroup = 0;
    timeTmp = 0;

    // create a sliding window over the hits and trigger an event
    // when the number of hits in the trigger window is
    // equal to or greater than the trigger threshold
    hits++;
    // check that the event has passed the trigger threshold
    if (hits < triggerThreshold) continue;
    // check that n hits (where n is the trigger threshold)
    // occurred within the trigger window
    double dt = pmtARRAY[pmtIndex][0] - pmtARRAY[hits - triggerThreshold][0];
    if (dt > triggerWindow) continue;
    // assign the sixth hit in a cluster of length triggerWindow as the trigger
    // (clusterTime is the the time of the 6th hit)
    for (unsigned long jj = 0; jj < clusterTime.size(); jj++) {
      // window of 800ns

      if (fabs(time - clusterTime[jj]) < postTriggerWindow) {
        oldGroup += 1;
        // This part get called only on second run through
        if (time < clusterTime[jj]) {
          clusterTime[jj] = time;
        }
      } else {
        timeTmp = time;
      }
    }
    if (oldGroup == 0) {
      if (nSubEvents == 0) {
        clusterTime.pop_back();  // Remove unrealistic time and provide better
                                 // alternative
        clusterTime.push_back(timeTmp);
      } else {
        clusterTime.push_back(timeTmp);
      }
      nSubEvents += 1;
    }
  }
  // std::sort(clusterTime.begin(), clusterTime.end());

  timeTmp = 0.;

  std::vector<double> idGroup, tGroup, qGroup, isDarkHit;
  bool itsThere;
  int goHere;

  for (int kk = 0; kk < nSubEvents; kk++) {
    DS::EV *ev = ds->AddNewEV();
    DS::PMT *pmt;

    if (kk == 0) {
      ev->SetDeltaT(clusterTime[kk]);
    } else {
      ev->SetDeltaT(clusterTime[kk] - clusterTime[kk - 1]);
    }

    ev->SetCalibratedTriggerTime((clusterTime[kk]));
    ev->SetID(kk);  // fEventCounter
    //            ev->SetUniqueID(fEventCounter);

    fEventCounter += 1;
    totalQ = 0.0;

    for (unsigned long pmtIndex = 0; pmtIndex < pmtARRAY.size(); pmtIndex++) {
      time = pmtARRAY[pmtIndex][0];

      if (time - clusterTime[kk] < postTriggerWindow && time - clusterTime[kk] >= preTriggerWindow) {
        /*if (pmtARRAY[pmtIndex][2] != oldID){
         timeTmp = time;
         pmt = ev->AddNewPMT();
         pmt->SetID(int(pmtARRAY[pmtIndex][2]));
         pmtQ = 0.0;
         }

         pmtQ += pmtARRAY[pmtIndex][1];
         totalQ += pmtARRAY[pmtIndex][1];

         //Set an offset of 200 ns for the PMT time and have a relative PMT time
         // Removed offset
         pmt->SetTime(timeTmp-clusterTime[kk]);
         pmt->SetCharge(pmtQ);
         oldID = pmtARRAY[pmtIndex][2];*/

        // check if the pmt id has existed within a group of subevent
        itsThere = false;
        for (unsigned long cc = 0; cc < idGroup.size(); cc++) {
          if (pmtARRAY[pmtIndex][2] == idGroup[cc]) {
            itsThere = true;
            goHere = cc;
            break;
          }
        }

        // if the PMT is already in the std::vector, the find its friend
        if (itsThere) {
          qGroup[goHere] += pmtARRAY[pmtIndex][1];
        }

        // if the PMT is not in the std::vector yet, add it to the std::vector
        else {
          idGroup.push_back(pmtARRAY[pmtIndex][2]);
          tGroup.push_back(pmtARRAY[pmtIndex][0] - clusterTime[kk]);
          qGroup.push_back(pmtARRAY[pmtIndex][1]);
          isDarkHit.push_back(pmtARRAY[pmtIndex][6]);  // is it a dark hit?
        }

        // accumulate total q within one subevent
        totalQ += pmtARRAY[pmtIndex][1];
      }
    }

    // we can then set these events on the PMT for one subevent
    for (unsigned long dd = 0; dd < idGroup.size(); dd++) {
      pmt = ev->AddNewPMT();
      pmt->SetID(idGroup[dd]);
      pmt->SetTime(tGroup[dd]);
      pmt->SetCharge(qGroup[dd]);
    }

    // resize all std::vectors for the next subevent
    idGroup.resize(0);
    tGroup.resize(0);
    qGroup.resize(0);

    // regster total charge of one subevent
    ev->SetTotalCharge(totalQ);
  }

  return Processor::OK;
}

}  // namespace RAT
