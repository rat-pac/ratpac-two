#include <RAT/InNetProducer.hh>
#include <RAT/InROOTProducer.hh>
#include <RAT/Log.hh>
#include <RAT/ProducerBlock.hh>
#include <RAT/RunManager.hh>

namespace RAT {

ProcBlock *ProducerBlock::mainBlock = nullptr;
std::vector<Producer *> ProducerBlock::fProducerList;

ProducerBlock::ProducerBlock() {}

ProducerBlock::~ProducerBlock() { Clear(); }

void ProducerBlock::Init(ProcBlock *theMainBlock) {
  mainBlock = theMainBlock;
  for (Producer *p : fProducerList) {
    p->SetMainBlock(mainBlock);
  }
  AppendProducer<RunManager>();
  AppendProducer<InROOTProducer>();
  AppendProducer<InNetProducer>();
}

void ProducerBlock::Clear() {
  for (unsigned i = 0; i < fProducerList.size(); i++) delete fProducerList[i];
  fProducerList.clear();
}

void ProducerBlock::BeginOfRun(DS::Run *run) {
  for (auto &prod : fProducerList) {
    prod->BeginOfRun(run);
  }
}

void ProducerBlock::EndOfRun(DS::Run *run) {
  for (auto &prod : fProducerList) {
    prod->EndOfRun(run);
  }
}

}  // namespace RAT
