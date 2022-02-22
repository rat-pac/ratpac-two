#ifndef __RAT_PMTWaveform__
#define __RAT_PMTWaveform__

#include <vector>
#include <RAT/PMTPulse.hh>
#include <TObject.h>
#include <TGraph.h>

namespace RAT {

class PMTWaveform {
public:

  PMTWaveform();
  virtual ~PMTWaveform();
  virtual double GetHeight(double time);

  std::vector<PMTPulse*> fPulse;

};

} // namespace RAT

#endif
