#ifndef __RAT_FitResult__
#define __RAT_FitResult__

#include <TObject.h>
#include <TVector3.h>
#include <string>
#include <vector>
#include <map>
#include <any>

namespace RAT {
namespace DS {

class FitResult : public TObject {
public:
  FitResult(std::string name) : TObject(),
    fit_name(name), fit_pass(0), fit_position(-1e9, -1e9, -1e9), fit_direction(0, 0, 0),
    fit_energy(-1e9), fit_time(-1e9) {}
  FitResult() : TObject(),
    fit_name(""), fit_pass(0), fit_position(-1e9, -1e9, -1e9), fit_direction(0, 0, 0),
    fit_energy(-1e9), fit_time(-1e9) {}
  virtual ~FitResult() {}

  // Fitter name
  virtual const std::string &GetFitterName() const { return fit_name; }
  virtual void SetFitterName(const std::string &_name) { fit_name = _name; }

  // Fitted Position
  virtual const TVector3 &GetPosition() const       { return fit_position; }
  virtual void SetPosition(const TVector3 &_pos)    { fit_position = _pos; }
  virtual const bool GetValidPosition()             { return valid_position; }
  virtual void SetValidPosition(const bool _valid)  { valid_position = _valid; }
  
  // Fitted Direction
  virtual const TVector3 &GetDirection() const      { return fit_direction; }
  virtual void SetDirection(const TVector3 &_dir)   { fit_direction = _dir; }
  virtual const bool GetValidDirection()            { return valid_direction; }
  virtual void SetValidDirection(const bool _valid) { valid_direction = _valid; }
  
  // Fitted Energy
  virtual const double GetEnergy()                  { return fit_energy; }
  virtual void SetEnergy(const double _energy)      { fit_energy = _energy; }
  virtual const bool GetValidEnergy()               { return valid_energy; }
  virtual void SetValidEnergy(const bool _valid)    { valid_energy = _valid; }
  
  // Fitted Time
  virtual const double GetTime()                    { return fit_time; }
  virtual void SetTime(const double _time)          { fit_time = _time; }
  virtual const bool GetValidTime()                 { return valid_time; }
  virtual void SetValidTime(const bool _valid)      { valid_time = _valid; }

  // Figures of Merit (arbitrary number)
  template <typename T>
  void SetFigureOfMerit(const std::string &name, T value){ figuresOfMerit[name] = value; }

  ClassDef(FitResult, 1)

protected:
  std::string fit_name;
  int         fit_pass;
  // Fit values
  TVector3    fit_position;
  TVector3    fit_direction;
  double      fit_energy;
  double      fit_time;
  // Fit validity
  bool valid_position;
  bool valid_direction;
  bool valid_energy;
  bool valid_time;

  // Figures of Merit
  std::map< std::string, std::any > figuresOfMerit;
};

} // namespace DS
} // namespace RAT

#endif
