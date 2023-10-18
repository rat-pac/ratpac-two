#ifndef __RAT_PMTWaveformGenerator__
#define __RAT_PMTWaveformGenerator__

#include <TObject.h>

#include <RAT/DS/MCPMT.hh>
#include <RAT/PMTWaveform.hh>
#include <vector>

namespace RAT {

class PMTWaveformGenerator {
 public:
  PMTWaveformGenerator(std::string modelName);
  virtual ~PMTWaveformGenerator();

  virtual PMTWaveform GenerateWaveforms(DS::MCPMT* mcpmt, double triggerTime);

  std::string fModelName;

  DBLinkPtr lpulse;
  std::string fPMTPulseType;
  std::string fPMTPulseShape;

  // Universal pulse parameters
  double fPMTPulseMin;
  double fPMTPulseOffset;
  double fPMTPulseTimeOffset;
  double fTerminationOhms;
  bool fPMTPulsePolarity;  // negative is true, positive is false

  // Shape parameters
  double fPMTPulseWidth;
  double fPMTPulseMean;

  // Explicit shape for data-driven pulses
  std::vector<double> fPMTPulseShapeTimes;
  std::vector<double> fPMTPulseShapeValues;
};

}  // namespace RAT

#endif
