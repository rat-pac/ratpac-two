#pragma once
#include <RAT/DB.hh>
#include <RAT/DS/FitResult.hh>
#include <RAT/FitterInputHandler.hh>

#include "mimir/ParamSet.hh"

namespace RAT::Mimir {
class FitStrategy {
 public:
  FitStrategy(RAT::FitterInputHandler *input_handler, const std::string &type, const std::string &name = "")
      : input_handler(input_handler), type_(type), name_(name) {}
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

  // Get the name of the fit strategy
  const std::string &Name() const { return name_; }

  // Get the type of the fit strategy
  const std::string &Type() const { return type_; }

 protected:
  RAT::FitterInputHandler *input_handler;
  const std::string type_;
  const std::string name_;
};

}  // namespace RAT::Mimir
