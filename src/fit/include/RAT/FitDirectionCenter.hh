#ifndef __RAT_FitDirectionCenter__
#define __RAT_FitDirectionCenter__

#include <RAT/FitterInputHandler.hh>
#include <RAT/Processor.hh>
#include <string>

namespace RAT {

namespace DS {
class Root;
class EV;
}  // namespace DS

class FitDirectionCenter : public Processor {
 public:
  FitDirectionCenter() : Processor("fitdirectioncenter"), inputHandler(){};
  virtual ~FitDirectionCenter() {}

  virtual void SetI(std::string param, int value);

  virtual void SetD(std::string param, double value);

  virtual void SetS(std::string param, std::string value);

  virtual Processor::Result Event(DS::Root *ds, DS::EV *ev);

 protected:
  std::vector<int> fPMTtype;  // Types of PMT to use in reconstruction.  If empty, uses all PMT types.
  int fVerbose = 0;  // Save FOMs in FitResult.  1 saves num_PMT.  2 also saves time_resid_low and time_resid_up.
  std::string fFitterName = "fitdirectioncenter";  // Default fitter name.  User can specify.
  std::string fPosFitter;                          // Position fitter from which to get reconstructed position.
  std::string fDirFitter;               // Direction fitter from which to get reconstructed direction for drive.
  std::string fCutMethod;               // "time"     selects time residuals in [fTimeResLow,fTimeResUp] ns.
                                        // "fraction" selects time residuals in [fTimeResFracLow,fTimeResFracUp].
  double fTimeResLow = -10.0;           // Lower cut on time residuals in ns
  double fTimeResUp = 50.0;             // Upper cut on time residuals in ns
  double fTimeResFracLow = 0.0;         // Lower cut on time residuals as a fraction in [0.0, 1.0)
  double fTimeResFracUp = 1.0;          // Upper cut on time residuals as a fraction in (0.0, 1.0]
  double fLightSpeed = 299.79 / 1.344;  // mm / s.  Speed of light in material.  Defaults to water.
  double fDrive = 0.0;                  // mm.  User-specified bias applied to event positions.
  double fFixedTime = 0.0;              // ns.  User-specified event time.
  TVector3 fFixedPosition;              // mm.  User-specified event position.
  std::string fPosMethod;
  FitterInputHandler inputHandler;
};

}  // namespace RAT

#endif  // __RAT_FitDirectionCenter__
