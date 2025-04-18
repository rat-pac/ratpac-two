#ifndef __RAT_InROOTProducer__
#define __RAT_InROOTProducer__

#include <RAT/Chroma.hh>
#include <RAT/DS/Run.hh>
#include <RAT/Producer.hh>
#include <globals.hh>
#include <string>

class G4UIcmdWithAString;
class G4UIcommand;

namespace RAT {

class InROOTProducer : public Producer {
 public:
  InROOTProducer();
  InROOTProducer(ProcBlock *block);
  virtual ~InROOTProducer();

  virtual bool ReadEvents(G4String filename);

  // override G4UImessenger (from Producer) methods
  virtual G4String GetCurrentValue(G4UIcommand *command);
  virtual void SetNewValue(G4UIcommand *command, G4String newValue);
  // BeginOfRun is called for all producers before a run, even if producer is not active. So we use a different name and
  // manually call it in the ReadEvents method.
  virtual void SetupRun(DS::Run *run);
  virtual void FinishRun(DS::Run *run);

 protected:
  void Init();

  G4UIcmdWithAString *readCmd;
  G4UIcommand *readDefaultCmd;
  Chroma *chroma;
  bool use_chroma;
};

}  // namespace RAT

#endif
