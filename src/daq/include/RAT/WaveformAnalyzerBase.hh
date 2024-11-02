/** @class WaveformAnalyzerBase
 *  Base class for PMT Waveform analyzers
 *
 *  @author James Shen <jierans@sas.upenn.edu>
 *
 * This is the base class for all waveform analyzers. It provides scaffolding
 * commonly shared by all analyzers.
 */

#ifndef __RAT_WaveformAnalyzerBase__
#define __RAT_WaveformAnalyzerBase__

#include <vector>

#include "RAT/DS/Digit.hh"
#include "RAT/DS/DigitPMT.hh"
#include "RAT/Digitizer.hh"
#include "RAT/Processor.hh"

namespace RAT {

class WaveformAnalyzerBase : public Processor {
 public:
  /**
   * Default constructor. Disabled as this class should only be inherited, not instantiated.
   */
  WaveformAnalyzerBase() = delete;

  /**
   * Create a new waveform analyzer.
   *
   * @param _procname Name of the processor. It correspond the processor registered in ProcBlockManager,
   *                  as well as the one called in macros.
   * @param _procname A configuration index used to specify a set of parameters. See Configure().
   */
  WaveformAnalyzerBase(std::string _procname, std::string config_name) : Processor(_procname) {
    Configure(config_name);
  }

  /**
   * Configure the processor. This method will be called upon processor instantiation.
   *
   * @param config_name  An index that is typically used to index table DIGITIZER_ANALYSIS in ratdb.
   *                     Although technically you can use it for anything.
   * In the base class this does nothing for now. But stuff can be added in the future if we would like
   * to set parameters for all processors.
   * */
  virtual void Configure(const std::string &config_name);

  /**
   * Functions that gets called for `/procset` lines in the macro. Base class calls allows you to set a
   * config name and call Configure().
   * */
  virtual void SetS(std::string param, std::string value) override;

  /**
   * Functions that gets called for `/procset` lines in the macro. Base class does nothing.
   * */
  virtual void SetD(std::string param, double value) override;
  virtual void SetI(std::string param, int value) override;

  /**
   * Function that sets up the digitizer object and calls DoAnalysis() appropriately.
   * @param digitpmt Pointer to the digitpmt that the current waveform correspond to.
   * @param pmtID    The ID of the current PMT. These may not correspond to real
   *                 PMTs, should an analysis is requested by a channel (LCN)
   *                 that does not map to a PMT.
   *
   * @param fDigitizer  digitizer that contains the waveforms.
   * This function sets up the following class variables:
   * fTimeStep: nanoseconds that correspond to a time step in the waveform.
   * fVoltageRes:    mV that correespond to one ADC in the waveform.
   * fTermOhms:      termination resistance of the voltage. Recall that charge
   *                 is voltage / termination integrated over time.
   * */
  virtual void RunAnalysis(DS::DigitPMT *digitpmt, int pmtID, Digitizer *fDigitizer);

  /**
   * Function that sets up the digitizer object and calls DoAnalysis() appropriately.
   * @param digitpmt Pointer to the digitpmt that the current waveform correspond to.
   * @param pmtID    The ID of the current PMT. These may not correspond to real
   *                 PMTs, should an analysis is requested by a channel (LCN)
   *                 that does not map to a PMT.
   * @param dsdigit  digitizer that contains the waveforms.
   *
   * This function sets up the following class variables:
   * fTimeStep: nanoseconds that correspond to a time step in the waveform.
   * fVoltageRes:    mV that correespond to one ADC in the waveform.
   * fTermOhms:      termination resistance of the voltage. Recall that charge
   *                 is voltage / termination integrated over time.
   * */
  virtual void RunAnalysis(DS::DigitPMT *digitpmt, int pmtID, DS::Digit *dsdigit);

  /**
   * Main function called for each event.
   * */
  Processor::Result Event(DS::Root *ds, DS::EV *ev) override;

  /**
   * Implementation of the waveform analysis.
   * @param digitpmt   Pointer to the digitpmt where analysis results are written to.
   * @param digitwfm   Waveform to analyze, in ADC units.
   *
   * See WaveformAnalysisLognormal for a concrete example. In order to write results,
   * instantiate a RAT::DS::WaveformAnalysisResult object via
   * `digitpmt->GetOrCreateWaveformAnalysisResult("procName")`.
   */
  virtual void DoAnalysis(DS::DigitPMT *digitpmt, const std::vector<UShort_t> &digitwfm) = 0;

 protected:
  // Digitizer Settings
  double fTimeStep;
  double fVoltageRes;
  double fTermOhms;
};

}  // namespace RAT

#endif
