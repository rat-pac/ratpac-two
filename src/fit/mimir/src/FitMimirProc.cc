#include <RAT/FitMimirProc.hh>
#include <mimir/PositionTime_PMTTypeTimeResidual.hh>
namespace RAT {

FitMimirProc::FitMimirProc() : Processor("mimir"), inputHandler() {}

void FitMimirProc::BeginOfRun(DS::Run *run) {
  strategy = std::make_unique<Mimir::PositionTime_PMTTypeTimeResidual>(&inputHandler);
  if (!strategy->Configure(RAT::DB::Get()->GetLink("FIT_MIMIR", ""))) {
    RAT::Log::Die("FitMimirProc: Failed to configure strategy " + strategy->Name());
  }
}

Processor::Result FitMimirProc::Event(DS::Root *ds, DS::EV *ev) {
  inputHandler.RegisterEvent(ev);
  Mimir::ParamSet result = strategy->Execute();
  RAT::DS::FitResult *ratds_result = new RAT::DS::FitResult();
  if (result.position_time.are_all_used()) {
    ratds_result->SetPosition(TVector3(result.position_time.components[0].value,
                                       result.position_time.components[1].value,
                                       result.position_time.components[2].value));
    ratds_result->SetTime(result.position_time.components[3].value);
  }
  if (result.direction.are_all_used()) {
    TVector3 direction;
    direction.SetMagThetaPhi(1.0, result.direction.components[0].value, result.direction.components[1].value);
    ratds_result->SetDirection(direction);
  }
  if (result.energy.are_all_used()) {
    ratds_result->SetEnergy(result.energy.components[0].value);
  }
  ev->AddFitResult(ratds_result);
  return Processor::Result(OK);
}

}  // namespace RAT
