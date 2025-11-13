#include <RAT/FitMimirProc.hh>
#include <mimir/Factory.hh>
#include <mimir/FitStep.hh>
#include <mimir/FitStrategy.hh>
namespace RAT {

FitMimirProc::FitMimirProc() : Processor("mimir"), inputHandler() {}

void FitMimirProc::BeginOfRun(DS::Run *run) {
  if (!configured) {
    info << "FitMimirProc: No strategy configured, using default from RATDB." << newline;
    DBLinkPtr lConfig = DB::Get()->GetLink("FIT_MIMIR", "");
    strategyName = lConfig->GetS("strategy");
    strategyConfig = lConfig->GetS("strategy_config");
  }
  // defer actual configuration until beginOfRun such that RATDB is set up correctly.
  Configure(strategyName, strategyConfig);
}

void FitMimirProc::Configure(const std::string &strategyName, const std::string &strategyConfig) {
  info << "Configuring FitMimirProc with strategy: " << strategyName << " and config: " << strategyConfig << newline;
  auto factory = Mimir::Factory<Mimir::FitStrategy>::GetInstance();
  strategy = factory.make_and_configure(strategyName, strategyConfig);
  if (!strategy) {
    Log::Die("Failed to create Mimir strategy: " + strategyName);
  }
  strategy->SetInputHandler(&inputHandler);
}

void FitMimirProc::SetS(std::string param, std::string value) {
  if (param == "strategy") {
    DB::ParseTableName(value, strategyName, strategyConfig);
    configured = true;
  }
}

Processor::Result FitMimirProc::Event(DS::Root *ds, DS::EV *ev) {
  inputHandler.RegisterEvent(ev);
  Mimir::ParamSet result = strategy->Execute();
  RAT::DS::FitResult *ratds_result = new RAT::DS::FitResult(name);
  if (result.position_time.are_all_used()) {
    ratds_result->SetPosition(TVector3(result.position_time.components[0].value,
                                       result.position_time.components[1].value,
                                       result.position_time.components[2].value));
    ratds_result->SetPosition(result.GetPosition());
    ratds_result->SetTime(result.GetTime());
    bool valid_xyzt = result.position_time.are_all_fit_valid();
    ratds_result->SetValidPosition(valid_xyzt);
    ratds_result->SetValidTime(valid_xyzt);
  }
  if (result.direction.are_all_used()) {
    ratds_result->SetDirection(result.GetDirection());
    ratds_result->SetValidDirection(result.direction.are_all_fit_valid());
  }
  if (result.energy.are_all_used()) {
    ratds_result->SetEnergy(result.GetEnergy());
    ratds_result->SetValidEnergy(result.energy.are_all_fit_valid());
  }
  ev->AddFitResult(ratds_result);
  return Processor::Result(OK);
}

}  // namespace RAT
