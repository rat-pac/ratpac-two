#include "RAT/Gen_RandomTrigger.hh"

#include <G4Event.hh>
#include <RAT/EventInfo.hh>
#include <RAT/Factory.hh>
#include <RAT/Log.hh>

#include "RAT/GLG4StringUtil.hh"
#include "RAT/GLG4TimeGen.hh"

Gen_RandomTrigger::Gen_RandomTrigger() : stateStr("") { timeGen = new GLG4TimeGen_Uniform(); }

Gen_RandomTrigger::~Gen_RandomTrigger() { delete timeGen; }

void Gen_RandomTrigger::GenerateEvent(G4Event *event) {
  RAT::EventInfo *exinfo = dynamic_cast<RAT::EventInfo *>(event->GetUserInformation());
  exinfo->extTriggerTime = 0.0;
  exinfo->extTriggerType = 7;
}

void Gen_RandomTrigger::ResetTime(double offset) { nextTime = timeGen->GenerateEventTime() + offset; }

void Gen_RandomTrigger::SetTimeState(G4String state) {
  if (timeGen)
    timeGen->SetState(state);
  else
    RAT::warn << "Gen_RandomTrigger error: Cannot set vertex state, no vertex "
                 "generator selected"
              << newline;
}

G4String Gen_RandomTrigger::GetTimeState() const {
  if (timeGen)
    return timeGen->GetState();
  else
    return G4String("GLG4Gen_RandomTrigger error: no time generator selected");
}
