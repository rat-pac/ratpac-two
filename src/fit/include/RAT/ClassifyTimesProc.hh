////////////////////////////////////////////////////////////////////////
///
/// \class   ClassifyTimesProc
///
/// \brief   Classifier processor for hit time residuals.
///
/// \author  Logan Lebanowski   llebanowski@berkeley.edu
///
/// \details Characteristic quantities are extracted from a distribution
///          of hit time residuals for a given event position.
///
/// REVISION HISTORY:
/// 2025/12/15 :  Logan Lebanowski  First version
///
////////////////////////////////////////////////////////////////////////

#ifndef __RAT_ClassifyTimesProc__
#define __RAT_ClassifyTimesProc__

#include <RAT/FitterInputHandler.hh>
#include <RAT/Processor.hh>
#include <string>

namespace RAT {

namespace DS {
class Root;
class EV;
}  // namespace DS

class ClassifyTimesProc : public Processor {
 public:
  ClassifyTimesProc() : Processor("classifytimes"), inputHandler(){};
  virtual ~ClassifyTimesProc() {}

  void BeginOfRun(DS::Run *run);

  virtual void SetI(std::string param, int value);

  virtual void SetD(std::string param, double value);

  virtual void SetS(std::string param, std::string value);

  virtual Processor::Result Event(DS::Root *ds, DS::EV *ev);

 protected:
  std::vector<std::string> fParamNames = {"ratio", "mean", "stddev", "skewness", "kurtosis"};

  std::vector<int> fPMTtype;                      // Types of PMT to use in classifier.  If empty, uses all PMT types.
  std::string fClassifierName = "classifytimes";  // Default classifier name.  User can specify.
  std::string fPosFitter;                         // Position fitter from which to get reconstructed position.
  double fLightSpeed = 0.0;  // mm / ns.  Speed of light in material.  Defaults to value in CLASSIFIER.ratdb
  double fFixedTime = 0.0;   // ns.  User-specified event time.
  TVector3 fFixedPosition;   // mm.  User-specified event position.
  std::string fPosMethod;
  DS::PMTInfo *fPMTInfo;
  FitterInputHandler inputHandler;

  // Parameters for the ratio of counts
  double fNumerTimeResLow =
      0.0;  // Numerator - Lower cut on time residuals in ns.  Defaults to value in CLASSIFIER.ratdb
  double fNumerTimeResUp =
      0.0;  // Numerator - Upper cut on time residuals in ns.  Defaults to value in CLASSIFIER.ratdb
  double fDenomTimeResLow = 0.0;  // Denominator - Lower cut on time residuals in ns
  double fDenomTimeResUp = 0.0;   // Denominator - Upper cut on time residuals in ns

  // Parameters for the four central moments in specified window
  int fVerbose =
      0;  // Save FOMs in ClassifierResult.  1 saves num_PMT's.  2 also saves time_resid_low and time_resid_up.
  std::string fCutMethod;        // "time"     selects time residuals in [fTimeResLow,fTimeResUp] ns.
                                 // "fraction" selects time residuals in [fTimeResFracLow,fTimeResFracUp].
  double fTimeResLow = 0.0;      // Lower cut on time residuals in ns
  double fTimeResUp = 0.0;       // Upper cut on time residuals in ns
  double fTimeResFracLow = 0.0;  // Lower cut on time residuals as a fraction in [0.0, 1.0)
  double fTimeResFracUp = 1.0;   // Upper cut on time residuals as a fraction in (0.0, 1.0]
};

}  // namespace RAT

#endif  // __RAT_ClassifyTimesProc__
