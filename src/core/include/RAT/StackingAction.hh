// fsutanto@umich.edu
// Apr 15,2018
// the class is added to kill secondary tracks
// that are produced by neutron capture on 158Gd

#ifndef StackingAction_h
#define StackingAction_h 1

#include "G4UserStackingAction.hh"
#include "globals.hh"

class StackingAction : public G4UserStackingAction {
 public:
  StackingAction();
  virtual ~StackingAction();

  virtual G4ClassificationOfNewTrack ClassifyNewTrack(const G4Track *);
};

#endif
