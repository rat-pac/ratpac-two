#include <RAT/DB.hh>
#include <RAT/Log.hh>
#include <mimir/Factory.hh>
#include <mimir/FitSteps.hh>

namespace Mimir {

bool FitSteps::Configure(RAT::DBLinkPtr db_link) {
  std::vector<std::string> configs = db_link->GetSArray("steps");
  fSteps.reserve(configs.size());
  auto factory = Mimir::Factory<Mimir::FitStrategy>::GetInstance();
  std::string strategyName, strategyConfig;
  for (const std::string& config : configs) {
    RAT::info << "Adding step " << config << " to the step list." << newline;
    RAT::DB::ParseTableName(config, strategyName, strategyConfig);
    std::unique_ptr<FitStrategy> strategy = factory.make_and_configure(strategyName, strategyConfig);
    fSteps.push_back(std::move(strategy));
  }
  return true;
}

void FitSteps::SetInputHandler(RAT::FitterInputHandler* handler) {
  FitStrategy::SetInputHandler(handler);
  for (const std::unique_ptr<FitStrategy>& step : fSteps) {
    step->SetInputHandler(handler);
  }
}

void FitSteps::Execute(ParamSet& params) {
  for (const std::unique_ptr<FitStrategy>& step : fSteps) {
    RAT::info << "Executing step " << step->GetName() << newline;
    step->Execute(params);
  }
}
}  // namespace Mimir
MIMIR_REGISTER_TYPE(Mimir::FitStrategy, Mimir::FitSteps, "FitSteps");
