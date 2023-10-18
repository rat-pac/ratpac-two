/* TrueDAQ is a "DAQ" that converts MC truth tracking information into multiple
 * triggers, and places those in new events (not subevents) to better simulate
 * data and produce combined datasets. Currently supports "single" events
 * (could include multiple initial particles but all expected to deposit E
 * quickly) and IBD. The "trigger" for "single" events and the IBD positron is
 * the highest energy step of the primary particles. The neutron capture
 * trigger is the time of the neutron capture. The vertex and direction for
 * these steps is used as well to define the event vertex and direction, and
 * trigger time is taken as the event time.  The energy of the event is summed
 * energy falling in the trigger "window", defined as the time between the end
 * of the previous window and the end of the current window. The trigger time
 * and deltaT are stored in the usual places.  The energy is stored in the
 * total charge entry. The pathfitter object stores the vertex information.
 *
 * TO BE VERY CLEAR: THIS IS NOT A ROBUST TRIGGER PROCESSOR I aim to expand it
 * one day based on triggering off energy deposition "pulses" so I can support
 * arbitrary numbers of triggers and integrate the pulse in the window to find
 * the energy but that day was not today.
 */
#include <G4ThreeVector.hh>
#include <RAT/DB.hh>
#include <RAT/DS/FitResult.hh>
#include <RAT/DetectorConstruction.hh>
#include <RAT/TrueDAQProc.hh>
#include <algorithm>
#include <map>
#include <vector>

namespace RAT {

TrueDAQProc::TrueDAQProc() : Processor("truedaq") {
  ldaq = DB::Get()->GetLink("DAQ", "TrueDAQ");
  fEventCounter = 0;
  fTriggerWindow = ldaq->GetD("trigger_window");
  fTriggerLockout = ldaq->GetD("trigger_lockout");
  fMaxHitTime = ldaq->GetD("max_hit_time");
}

Processor::Result TrueDAQProc::DSEvent(DS::Root *ds) {
  // This "DAQ" will convert tracking information into separate events
  // for coincidences and single events for singles
  // so that some element of trigger timing can be used
  // in coincidence analysis such as reactor discovery via IBD.
  // Anything that has a positron and a neutron as the only MCParticles will be
  // considered an IBD event. Any other events will be treated as single events.
  // Truth information for "sub"-MC events is stored in the PathFit structure
  // This is a disgusting abuse of the data structure so I'm not gonna bother
  // being very clean
  DS::MC *mc = ds->GetMC();
  // Prune the previous EV branchs if one exists
  if (ds->ExistEV()) {
    ds->PruneEV();
  }

  bool hasPositron = false;
  bool hasNeutron = false;
  bool isIBD = false;    // IBD events
  bool isDecay = false;  // Coincidence decays like BiPos, this doesn't do anything now
  bool isSingle = true;  // Default, everything else and stuff that doesn't fit
  // Loop over primary particles to determine what type of event this is
  for (int imcparticle = 0; imcparticle < mc->GetMCParticleCount(); imcparticle++) {
    DS::MCParticle *mcparticle = mc->GetMCParticle(imcparticle);
    if (mcparticle->GetPDGCode() == -11) {
      hasPositron = true;
    }
    if (mcparticle->GetPDGCode() == 2112) {
      hasNeutron = true;
    }
    // FIXME Add BiPo check
  }
  if (hasPositron && hasNeutron && mc->GetMCParticleCount() == 2) {
    isIBD = true;
    isSingle = false;
  }

  // IBD variables. We always fill positron first so don't need to consider
  // coming back to it
  bool foundPositron = false;
  bool foundNeutron = false;
  int neutronTrackIndex = -1;

  //"Singles" variables. We keep track of the number of MCParticles and the
  // appropriate track/step indices for most energetic point to use as vertex
  int mcParticleCount = 0;
  double mostEnergeticStepEnergy = 0;
  int mostEnergeticStepTrackIndex = -1;
  int mostEnergeticStepStepIndex = -1;

  // Set last trigger for events in this MC as t = 0
  double lastTrigger = -1 * (fTriggerWindow + fTriggerLockout);
  for (int imctrack = 0; imctrack < mc->GetMCTrackCount(); imctrack++) {
    // Ignore triggering on secondaries unless a decay is involved and always
    // ignore opticalphotons
    DS::MCTrack *mctrack = mc->GetMCTrack(imctrack);
    if ((mctrack->GetParentID() != 0 && !isDecay) || mctrack->GetPDGCode() == 0) {
      continue;
    }
    mcParticleCount++;

    // One day I'll update this to trigger on energy deposition just like we do
    // on hits but for now we'll be dumb and just trigger on the highest energy
    // step associated with primary particles IBD requires annoying checks to
    // maintain order in event structre
    if (isSingle || (isIBD && !foundPositron && !foundNeutron && mctrack->GetPDGCode() == -11)) {
      double prevKE = mctrack->GetMCTrackStep(0)->GetKE();
      for (int imctrackstep = 0; imctrackstep < mctrack->GetMCTrackStepCount(); imctrackstep++) {
        DS::MCTrackStep *mctrackstep = mctrack->GetMCTrackStep(imctrackstep);

        // If step is most energetic, set variables. If gamma, look at delta KE
        // to see what Compton e- energy is so that we don't have to descend the
        // tracks
        if (mctrackstep->GetDepositedEnergy() > mostEnergeticStepEnergy ||
            (mctrack->GetPDGCode() == 22 && (prevKE - mctrackstep->GetKE()) > mostEnergeticStepEnergy)) {
          mostEnergeticStepEnergy = mctrackstep->GetDepositedEnergy();
          if (mctrack->GetPDGCode() == 22) {
            mostEnergeticStepEnergy = prevKE - mctrackstep->GetKE();
          }
          mostEnergeticStepTrackIndex = imctrack;
          mostEnergeticStepStepIndex = imctrackstep;
        }
        prevKE = mctrackstep->GetKE();
      }

      if (isIBD) {
        foundPositron = true;
        // Alter index variable to get to the IBD neutron on the next loop if
        // we've seen it
        if (neutronTrackIndex != -1) {
          imctrack = neutronTrackIndex - 1;
        }
      }
    }

    // For IBD, store neutron track index if it comes before the positron.
    // foundNeutron stays off since that's to indicate we're ready to build that
    // "ev"
    if (isIBD && !foundPositron && !foundNeutron && mctrack->GetPDGCode() == 2112 && mctrack->GetParentID() == 0) {
      neutronTrackIndex = imctrack;
    }
    // For IBD, if we haven't found the positron track, then we continue looping
    // until we do without triggering
    if (isIBD && !foundPositron) {
      continue;
    }
    // For "singles", we need to check all MCParticles before we can do the
    // vertex
    if (isSingle && mcParticleCount < mc->GetMCParticleCount()) {
      continue;
    }

    // Set "event" variables
    double tt = -100000000;
    TVector3 vertex(0, 0, 0);
    TVector3 dir(0, 0, 0);
    // For IBD, if we've found the positron, now we take the neutron capture
    // position (last step)
    if (isIBD && foundPositron && !foundNeutron && mctrack->GetPDGCode() == 2112 && mctrack->GetParentID() == 0) {
      tt = mctrack->GetMCTrackStep(mctrack->GetMCTrackStepCount() - 1)->GetGlobalTime();
      vertex = mctrack->GetMCTrackStep(mctrack->GetMCTrackStepCount() - 1)->GetEndpoint();
      // No direction associated with a capture so *shrug*
      dir = mctrack->GetMCTrackStep(mctrack->GetMCTrackStepCount() - 1)->GetMomentum().Unit();
      foundNeutron = true;
    }

    // For "singles", if we have all the MCParticles, we know the most energetic
    // step from a primary. Do same for IBD prompt positron
    if ((isSingle && mcParticleCount == mc->GetMCParticleCount()) ||
        (isIBD && foundPositron && mctrack->GetPDGCode() == -11 && mctrack->GetParentID() == 0)) {
      tt = mc->GetMCTrack(mostEnergeticStepTrackIndex)->GetMCTrackStep(mostEnergeticStepStepIndex)->GetGlobalTime();
      vertex = mc->GetMCTrack(mostEnergeticStepTrackIndex)->GetMCTrackStep(mostEnergeticStepStepIndex)->GetEndpoint();
      // This assumes forward scattering if most energetic step is from gamma.
      // We take the displacement std::vector from the previous indexed step since if
      // we lost all our energy in this step, we won't have a momentum and also
      // this was the direction of travel as the energy was deposited, since we
      // save the step endpoints
      dir = (mc->GetMCTrack(mostEnergeticStepTrackIndex)->GetMCTrackStep(mostEnergeticStepStepIndex)->GetEndpoint() -
             mc->GetMCTrack(mostEnergeticStepTrackIndex)->GetMCTrackStep(mostEnergeticStepStepIndex - 1)->GetEndpoint())
                .Unit();
    }

    if (tt >= lastTrigger + fTriggerWindow + fTriggerLockout) {
      DS::EV *ev = ds->AddNewEV();
      ev->SetID(fEventCounter++);

      double energy = 0;
      for (int jmctrack = 0; jmctrack < mc->GetMCTrackCount(); jmctrack++) {
        DS::MCTrack *mctrack2 = mc->GetMCTrack(jmctrack);
        double initTime = mctrack2->GetMCTrackStep(0)->GetGlobalTime();
        double finalTime = mctrack2->GetMCTrackStep(mctrack2->GetMCTrackStepCount() - 1)->GetGlobalTime();
        bool allInWindow = initTime >= lastTrigger + fTriggerWindow + fTriggerLockout &&
                           initTime < tt + fTriggerWindow + fTriggerLockout &&
                           finalTime >= lastTrigger + fTriggerWindow + fTriggerLockout &&
                           finalTime < tt + fTriggerWindow + fTriggerLockout;
        bool allOutWindow =
            (initTime < lastTrigger + fTriggerWindow + fTriggerLockout &&
             finalTime < lastTrigger + fTriggerWindow + fTriggerLockout) ||
            (initTime > tt + fTriggerWindow + fTriggerLockout && finalTime > tt + fTriggerWindow + fTriggerLockout);

        // Ignore optical photons and stuff that's entirely out of the window
        if (mctrack2->GetPDGCode() == 0 || mctrack2->GetParticleName() == "opticalphoton" || allOutWindow) {
          continue;
        }

        if (allInWindow) {
          energy += mctrack2->GetDepositedEnergy();
        }
        // Otherwise we have to step through the tracks and find the inwindow
        // bits
        else {
          for (int jmctrackstep = 0; jmctrackstep < mctrack2->GetMCTrackStepCount(); jmctrackstep++) {
            DS::MCTrackStep *mctrackstep2 = mctrack2->GetMCTrackStep(jmctrackstep);
            // If time of MCStep is after previous trigger window ends and
            // before current trigger window ends, count as deposited in this
            // event
            if (mctrackstep2->GetGlobalTime() >= lastTrigger + fTriggerWindow + fTriggerLockout &&
                mctrackstep2->GetGlobalTime() < tt + fTriggerWindow + fTriggerLockout) {
              energy += mctrackstep2->GetDepositedEnergy();
            }
          }
        }
      }

      ev->SetCalibratedTriggerTime(tt);
      ev->SetDeltaT(tt - lastTrigger);
      lastTrigger = tt;
      ev->SetTotalCharge(energy);

      // Fill pathfit structure
      DS::FitResult *fit = new DS::FitResult("TrueDAQ");
      fit->SetPosition(vertex);
      fit->SetTime(tt);
      fit->SetDirection(dir);
      ev->AddFitResult(fit);

      // Only one trigger window for a single event, so if we got here we're
      // done
      if (isSingle) {
        break;
      }
    }
    // If we got here with both booleans flipped, then we are also done
    if (isIBD && foundPositron && foundNeutron) {
      break;
    }
  }
  return Processor::OK;
}

void TrueDAQProc::SetD(std::string param, double value) {
  if (param == "trigger_window") {
    fTriggerWindow = value;
  } else if (param == "trigger_lockout") {
    fTriggerLockout = value;
  } else if (param == "max_hit_time") {
    fMaxHitTime = value;
  } else {
    throw ParamUnknown(param);
  }
}

void TrueDAQProc::SetI(std::string param, int value) { throw ParamUnknown(param); }

}  // namespace RAT
