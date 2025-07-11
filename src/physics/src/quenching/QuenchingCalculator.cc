#include <RAT/Gsim.hh>
#include <RAT/AppliedQuenchingModel.hh>
#include <RAT/QuenchingCalculator.hh>
#include <RAT/NaiveQuenchingCalculator.hh>
#include <RAT/IntegratedQuenchingCalculator.hh>
#include <RAT/FixedTrapezoidalQuadrature.hh>
#include <RAT/AdaptiveSimpsonQuadrature.hh>


QuenchingCalculator::QuenchingCalculator(BirksLaw model) : model(model) { /* */
}

QuenchingCalculator* QuenchingCalculator::BuildQuenchingCalculator() {
  QuenchingCalculator* quenching_calculator;

  RAT::DB *db = RAT::DB::Get();
  RAT::DBLinkPtr tbl = db->GetLink("QUENCHING");
  std::string selection = tbl->GetS("model");
  BirksLaw model;
  if (selection == "birks") {
    model = BirksLaw();
  } else {
    // no such quenching model
    std::string msg = "Invalid quenching model: " + selection;
    RAT::Log::Die(msg);
  }
  std::string strategy = tbl->GetS("strategy");
  if (strategy == "naive") {
    quenching_calculator = new NaiveQuenchingCalculator(model);
  } else if (strategy == "integrated") {
    std::string method = tbl->GetS("integration");
    Quadrature *quadrature;
    if (method == "fixed") {
      // TODO
      double resolution = tbl->GetD("resolution");
      quadrature = new FixedTrapezoidalQuadrature(resolution);
    } else if (method == "adaptive") {
      double tolerance = tbl->GetD("tolerance");
      quadrature = new AdaptiveSimpsonQuadrature(tolerance);
    } else {
      // no such integration method
      std::string msg = "Invalid integration method: " + method;
      RAT::Log::Die(msg);
    }
    quenching_calculator = new IntegratedQuenchingCalculator(model, quadrature);
  } else {
    // no such quenching calculation strategy
    std::string msg = "Invalid quenching calculation strategy: " + strategy;
    RAT::Log::Die(msg);
  }
  return quenching_calculator;
}
