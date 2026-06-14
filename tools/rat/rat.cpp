#include <RAT/AnyParse.hh>
#include <RAT/FatalError.hh>
#include <RAT/Rat.hh>

int main(int argc, char **argv) {
  try {
    auto parser = new RAT::AnyParse(argc, argv);
    // Additional experiment arguments
    auto experiment = RAT::Rat(parser, argc, argv);
    // Run experiment
    experiment.Begin();
    experiment.Report();
  } catch (const RAT::FatalError &e) {
    // Message has already been logged by Log::Die(); just propagate the code.
    return e.return_code;
  }
  return 0;
}
