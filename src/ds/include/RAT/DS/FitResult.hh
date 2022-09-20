#ifndef __RAT_FitResult__
#define __RAT_FitResult__

#include <TObject.h>
#include <TVector3.h>

#include <RAT/DS/Classifier.hh>
#include <map>
#include <string>
#include <vector>

namespace RAT {
namespace DS {

class FitResult : public TObject {
 public:
  FitResult(std::string name)
      : TObject(),
        fit_name(name),
        fit_pass(0),
        fit_position(-1e9, -1e9, -1e9),
        fit_direction(0, 0, 0),
        fit_energy(-1e9),
        fit_time(-1e9) {}
  FitResult()
      : TObject(),
        fit_name(""),
        fit_pass(0),
        fit_position(-1e9, -1e9, -1e9),
        fit_direction(0, 0, 0),
        fit_energy(-1e9),
        fit_time(-1e9) {}
  virtual ~FitResult() {}

  // Fitter name
  virtual const std::string &GetFitterName() const { return fit_name; }
  virtual void SetFitterName(const std::string &_name) { fit_name = _name; }

  // Fitted Position
  virtual const TVector3 &GetPosition() const { return fit_position; }
  virtual void SetPosition(const TVector3 &_pos) {
    fit_position = _pos;
    SetValidPosition(true);
    SetEnablePosition(true);
  }
  virtual const bool GetValidPosition() { return valid_position; }
  virtual void SetValidPosition(const bool _valid) { valid_position = _valid; }
  virtual const bool GetEnablePosition() { return enable_position; }
  virtual void SetEnablePosition(const bool _enable) { enable_position = _enable; }

  // Fitted Direction
  virtual const TVector3 &GetDirection() const { return fit_direction; }
  virtual void SetDirection(const TVector3 &_dir) {
    fit_direction = _dir;
    SetValidDirection(true);
    SetEnableDirection(true);
  }
  virtual const bool GetValidDirection() { return valid_direction; }
  virtual void SetValidDirection(const bool _valid) { valid_direction = _valid; }
  virtual const bool GetEnableDirection() { return enable_direction; }
  virtual void SetEnableDirection(const bool _enable) { enable_direction = _enable; }

  // Fitted Energy
  virtual const double GetEnergy() { return fit_energy; }
  virtual void SetEnergy(const double _energy) {
    fit_energy = _energy;
    SetValidEnergy(true);
    SetEnableEnergy(true);
  }
  virtual const bool GetValidEnergy() { return valid_energy; }
  virtual void SetValidEnergy(const bool _valid) { valid_energy = _valid; }
  virtual const bool GetEnableEnergy() { return enable_energy; }
  virtual void SetEnableEnergy(const bool _enable) { enable_energy = _enable; }

  // Fitted Time
  virtual const double GetTime() { return fit_time; }
  virtual void SetTime(const double _time) {
    fit_time = _time;
    SetValidTime(true);
    SetEnableTime(true);
  }
  virtual const bool GetValidTime() { return valid_time; }
  virtual void SetValidTime(const bool _valid) { valid_time = _valid; }
  virtual const bool GetEnableTime() { return enable_time; }
  virtual void SetEnableTime(const bool _enable) { enable_time = _enable; }

  // Figures of Merit (arbitrary number)
  void SetIntFigureOfMerit(const std::string &name, int value) { intFiguresOfMerit[name] = value; }
  void SetBoolFigureOfMerit(const std::string &name, bool value) { boolFiguresOfMerit[name] = value; }
  void SetDoubleFigureOfMerit(const std::string &name, double value) { doubleFiguresOfMerit[name] = value; }

  int GetIntFigureOfMerit(const std::string &name) { return intFiguresOfMerit[name]; }
  bool GetBoolFigureOfMerit(const std::string &name) { return boolFiguresOfMerit[name]; }
  double GetDoubleFigureOfMerit(const std::string &name) { return doubleFiguresOfMerit[name]; }

  // Figures of Merit
  std::map<std::string, int> intFiguresOfMerit;
  std::map<std::string, bool> boolFiguresOfMerit;
  std::map<std::string, double> doubleFiguresOfMerit;

  // Classification
  std::map<std::string, Classifier> classifiers;

  ClassDef(FitResult, 1);

 protected:
  std::string fit_name;
  int fit_pass;
  // Fit values
  TVector3 fit_position;
  TVector3 fit_direction;
  double fit_energy;
  double fit_time;
  // Fit validity
  bool valid_position = false;
  bool valid_direction = false;
  bool valid_energy = false;
  bool valid_time = false;
  // Fit enabled
  bool enable_position = false;
  bool enable_direction = false;
  bool enable_energy = false;
  bool enable_time = false;
};

}  // namespace DS
}  // namespace RAT

#endif
