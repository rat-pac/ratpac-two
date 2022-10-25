#ifndef __RAT_ProducerBlock__
#define __RAT_ProducerBlock__

#include <RAT/ProcBlock.hh>
#include <RAT/Producer.hh>
#include <vector>

namespace RAT {

class ProcBlock;

class ProducerBlock {
 public:
  /** Create an empty block of processors. */
  ProducerBlock();
  virtual ~ProducerBlock();

  void Init(ProcBlock *theMainBlock);
  virtual void BeginOfRun(DS::Run *run);
  virtual void EndOfRun(DS::Run *run);
  virtual void Clear();

  template <class T>
  static void AppendProducer() {
    fProducerList.push_back(new T(mainBlock));
  }

  static std::vector<Producer *> fProducerList;
  static ProcBlock *mainBlock;
};

}  // namespace RAT

#endif
