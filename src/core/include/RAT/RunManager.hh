/**
 * @class RAT::RunManager
 * @author William Seligman (31-Jul-2006)
 *
 * @detail Set up the G4RunManager environment, before initializing the
 * main RAT simulation in RAT::Gsim.
 */

#ifndef __RAT_RunManager__
#define __RAT_RunManager__
#include <RAT/Producer.hh>

class G4RunManager;
class G4VisManager;

namespace RAT {

class Gsim;
class ProcBlock;

class RunManager : public Producer {
 public:
  RunManager();
  RunManager(ProcBlock *theMainBlock);
  virtual ~RunManager();

 protected:
  void Init();

  G4RunManager *theRunManager;
  ProcBlock *mainBlock;
  Gsim *ratGsim;

  G4VisManager *theVisManager;
};

}  // namespace RAT

#endif  // __RAT_RunManager__
