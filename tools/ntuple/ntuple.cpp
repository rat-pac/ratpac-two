#include <TFile.h>
#include <TTree.h>

#include <RAT/AnyParse.hh>
#include <RAT/DB.hh>
#include <RAT/DBTable.hh>
#include <RAT/Log.hh>
#include <RAT/OutNtupleProc.hh>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
  auto parser = new RAT::AnyParse(argc, argv);
  parser->SetHelpLine("[options] inputfile.root");
  parser->AddArgument("verbose", false, "v", 0, "Verbosity", RAT::ParseInt);
  parser->AddArgument("tracking", false, "t", 0, "Include Tracking", RAT::ParseInt);
  parser->AddArgument("mcparticles", false, "m", 0, "Include All MC Particles", RAT::ParseInt);
  parser->AddArgument("pmthits", true, "p", 0, "Include PMT Hits", RAT::ParseInt);
  parser->AddArgument("untriggered", true, "u", 0, "Include Untriggered MC Events", RAT::ParseInt);
  parser->Parse();

  for (auto &filename : parser->Positionals) {
    std::string outputname = filename;
    if (outputname.find(".root") != std::string::npos) {
      outputname.replace(outputname.find(".root"), 5, ".ntuple.root");
    }
    if (parser->GetValue("verbose", false)) {
      std::cout << "Reading " << filename << std::endl;
    }
    TFile *infile = new TFile(filename.c_str());
    TTree *T = (TTree *)infile->Get("T");
    TTree *run = (TTree *)infile->Get("runT");

    RAT::DS::Root *ds = new RAT::DS::Root();
    T->SetBranchAddress("ds", &ds);

    std::string macro = static_cast<std::string>(dynamic_cast<TObjString *>(infile->Get("macro"))->GetString().Data());

    RAT::Log::AddMacro(macro);

    RAT::OutNtupleProc proc;
    proc.options.tracking = parser->GetValue("tracking", false);
    proc.options.mcparticles = parser->GetValue("mcparticles", false);
    proc.options.pmthits = parser->GetValue("pmthits", false);
    proc.options.untriggered = parser->GetValue("untriggered", false);

    run->SetBranchAddress("run", &proc.runBranch);
    run->GetEvent(0);

    proc.OpenFile(outputname);
    for (int ev = 0; ev < T->GetEntries(); ev++) {
      T->GetEvent(ev);
      proc.DSEvent(ds);
    }
  }
}
