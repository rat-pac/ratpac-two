#pragma once
#include <RAT/DB.hh>
#include <RAT/DS/FitResult.hh>
#include <RAT/FitterInputHandler.hh>

#include "mimir/ParamSet.hh"

namespace RAT::Mimir {
class FitStrategy {
 public:
  FitStrategy() = default;
  virtual ~FitStrategy() = default;

  // Configure the fit strategy with a database link
  virtual bool Configure(RAT::DBLinkPtr db_link) { return true; }

  ParamSet ConstructSeedFromInputHandler() {
    ParamSet params;
    std::vector<double> xyzt = {0, 0, 0, 0};
    if (input_handler->ValidSeedPosition()) {
      TVector3 seed_pos = input_handler->GetSeedPosition();
      xyzt[0] = seed_pos.X();
      xyzt[1] = seed_pos.Y();
      xyzt[2] = seed_pos.Z();
    }
    if (input_handler->ValidSeedTime()) {
      xyzt[3] = input_handler->GetSeedTime();
    }
    params.position_time.set_values(xyzt);
    if (input_handler->ValidSeedDirection()) {
      TVector3 seed_dir = input_handler->GetSeedDirection();
      std::vector<double> seed_theta_phi = {seed_dir.Theta(), seed_dir.Phi()};
      params.direction.set_values(seed_theta_phi);
    }
    if (input_handler->ValidSeedEnergy()) {
      params.energy.set_values({input_handler->GetSeedEnergy()});
    }
    return params;
  }

  // Execute the fit strategy
  ParamSet Execute() {
    ParamSet params = ConstructSeedFromInputHandler();
    Execute(params);
    return params;
  }

  virtual void Execute(ParamSet &params) = 0;

  void SetInputHandler(RAT::FitterInputHandler *handler) { input_handler = handler; }

  void SetName(const std::string &_name) { name = _name; }

  const std::string &GetName() const { return name; }

 protected:
  RAT::FitterInputHandler *input_handler;
  std::string name;
};

}  // namespace RAT::Mimir
