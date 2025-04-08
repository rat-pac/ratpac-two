////////////////////////////////////////////////////////////////////////
///
/// \class   FitDirectionCenterProc
///
/// \brief   Fitter processor for reconstructing event direction.
///
/// \author  Logan Lebanowski   llebanowski@berkeley.edu
///
/// \details Event direction is reconstructed as the average of the
///          vectors from the event position to the hit PMT positions.
///
/// REVISION HISTORY:
/// 2025/02/27 :  Logan Lebanowski  First version
///
////////////////////////////////////////////////////////////////////////

#ifndef __RAT_FitDirectionCenterProc__
#define __RAT_FitDirectionCenterProc__

#include <RAT/FitterInputHandler.hh>
#include <RAT/Processor.hh>
#include <string>

namespace RAT {

namespace DS {
class Root;
class EV;
}  // namespace DS

class FitDirectionCenterProc : public Processor {
 public:
  FitDirectionCenterProc() : Processor("fitdirectioncenter"), inputHandler(){};
  virtual ~FitDirectionCenterProc() {}

  void BeginOfRun(DS::Run *run);

  virtual void SetI(std::string param, int value);

  virtual void SetD(std::string param, double value);

  virtual void SetS(std::string param, std::string value);

  virtual Processor::Result Event(DS::Root *ds, DS::EV *ev);

 protected:
  std::vector<int> fPMTtype;  // Types of PMT to use in reconstruction.  If empty, uses all PMT types.
  int fVerbose = 0;  // Save FOMs in FitResult.  1 saves num_PMT.  2 also saves time_resid_low and time_resid_up.
  std::string fFitterName = "fitdirectioncenter";  // Default fitter name.  User can specify.
  std::string fPosFitter;                          // Position fitter from which to get reconstructed position.
  std::string fDirFitter;        // Direction fitter from which to get reconstructed direction for drive.
  std::string fCutMethod;        // "time"     selects time residuals in [fTimeResLow,fTimeResUp] ns.
                                 // "fraction" selects time residuals in [fTimeResFracLow,fTimeResFracUp].
  double fTimeResLow = -10.0;    // Lower cut on time residuals in ns
  double fTimeResUp = 50.0;      // Upper cut on time residuals in ns
  double fTimeResFracLow = 0.0;  // Lower cut on time residuals as a fraction in [0.0, 1.0)
  double fTimeResFracUp = 1.0;   // Upper cut on time residuals as a fraction in (0.0, 1.0]
  double fLightSpeed = 0.0;      // mm / ns.  Speed of light in material.  Defaults to value in FITTER.ratdb
  double fDrive = 0.0;           // mm.  User-specified bias applied to event positions.
  double fFixedTime = 0.0;       // ns.  User-specified event time.
  TVector3 fFixedPosition;       // mm.  User-specified event position.
  std::string fPosMethod;
  DS::PMTInfo *fPMTInfo;
  FitterInputHandler inputHandler;
};

}  // namespace RAT

#endif  // __RAT_FitDirectionCenterProc__
