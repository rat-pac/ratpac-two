#ifndef __RAT_PMTWaveform__
#define __RAT_PMTWaveform__

#include <TGraph.h>
#include <TObject.h>

#include <RAT/PMTPulse.hh>
#include <vector>

namespace RAT {

class PMTWaveform {
 public:
  PMTWaveform();
  virtual ~PMTWaveform();
  virtual double GetHeight(double time);

  std::vector<PMTPulse *> fPulse;
};

}  // namespace RAT

#endif
