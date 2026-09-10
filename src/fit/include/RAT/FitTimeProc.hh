////////////////////////////////////////////////////////////////////////
///
/// \class   FitTimeProc
///
/// \brief   Fitter processor for reconstructing event time.
///
/// \author  Logan Lebanowski   llebanowski@berkeley.edu
///
/// \details Event time is reconstructed as the median of the time
///          residuals from an event position to the hit PMT positions.
///
/// REVISION HISTORY:
/// 2026/08/07 :  Logan Lebanowski  First version
///
////////////////////////////////////////////////////////////////////////

#ifndef __RAT_FitTimeProc__
#define __RAT_FitTimeProc__

#include <RAT/FitterInputHandler.hh>
#include <RAT/Processor.hh>
#include <RAT/TransitTimeCalculator.hh>
#include <string>

namespace RAT {

namespace DS {
class Root;
class EV;
}  // namespace DS

class FitTimeProc : public Processor {
 public:
  FitTimeProc() : Processor("fittime"), inputHandler(){};
  virtual ~FitTimeProc() {}

  void BeginOfRun(DS::Run *run);

  virtual void SetI(std::string param, int value);

  virtual void SetD(std::string param, double value);

  virtual void SetS(std::string param, std::string value);

  virtual Processor::Result Event(DS::Root *ds, DS::EV *ev);

 private:
  DS::PMTInfo *fPMTInfo;
  std::vector<int> fPMTtype;   // Types of PMT to use in reconstruction.  If empty, uses all PMT types.
  std::string fFitLabel = "";  // Label for the fit result.  User can specify.
  double fLightSpeed;          // [mm/ns].  Defaults to value in FIT_COMMON table.
  double fWavelength = 400.0;  // [nm]
  bool fSetWavelength = false;
  TVector3 fPosition;          // [mm].  User-specified event position.
  double fMaxHitTime = -9999;  // [ns].  Optional hit time limits - ineffective when fMaxHitTime <= fMinHitTime.
  double fMinHitTime = 9999;   // [ns].
  bool fSetMaxHitTime = false;
  bool fSetMinHitTime = false;

 protected:
  FitterInputHandler inputHandler;
  std::unique_ptr<RAT::TransitTimeCalculator> fTransitTimeCalc;
};

}  // namespace RAT

#endif  // __RAT_FitTimeProc__
