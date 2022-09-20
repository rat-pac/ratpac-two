#ifndef __RAT_ProcBlockManager__
#define __RAT_ProcBlockManager__

#include <G4String.hh>
#include <G4UImessenger.hh>
#include <RAT/ProcAllocator.hh>
#include <map>
#include <stack>
#include <string>

namespace RAT {

class ProcBlock;
class UserProcManager;

class ProcBlockManager : public G4UImessenger {
 public:
  ProcBlockManager(ProcBlock *theMainBlock);
  virtual ~ProcBlockManager();

  // override G4UImessenger methods to parse commands
  virtual G4String GetCurrentValue(G4UIcommand *command);
  virtual void SetNewValue(G4UIcommand *command, G4String newValue);

  // Methods implementing the commands
  virtual bool DoProcCmd(std::string procname, bool last = false);
  virtual void DoProcSetCmd(std::string cmdstring);

  // Processor Table
  // template <class T> void AppendProcessor();
  static std::map<std::string, ProcAllocator *> procAllocators;

  template <class T>
  static void AppendProcessor() {
    ProcAllocator *pa = new ProcAllocatorTmpl<T>;
    procAllocators[pa->operator()()->GetName()] = pa;
  }

 protected:
  ProcBlock *mainBlock;  // Convenience, non-NULL when we are at the main
  // block in analysis construction
  std::stack<ProcBlock *> blocks;

  Processor *lastProc;

  // interface commands
  G4UIcommand *procCmd;
  G4UIcommand *procLastCmd;
  G4UIcommand *setCmd;
};

}  // namespace RAT

#endif
