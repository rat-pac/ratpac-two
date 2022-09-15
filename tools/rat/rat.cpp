#include <RAT/AnyParse.hh>
#include <RAT/Rat.hh>

int main(int argc, char **argv) {
  auto parser = new RAT::AnyParse(argc, argv);
  // Additional experiment arguments
  auto experiment = RAT::Rat(parser, argc, argv);
  // Run experiment
  experiment.Begin();
  experiment.Report();
}
