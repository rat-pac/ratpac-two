#include <TFile.h>
#include <TLeaf.h>
#include <TTree.h>
#include <sys/stat.h>

#include <RAT/BNLOpWLSData.hh>
#include <RAT/Log.hh>
#include <iostream>
#include <vector>

BNLOpWLSData::BNLOpWLSData(std::string fname) { SetExEmData(fname); }

BNLOpWLSData::~BNLOpWLSData() {}

void *BNLOpWLSData::GetPointerToValue(TBranch *theBranch, int entry, const char *name) {
  theBranch->GetEntry(entry);
  TLeaf *theLeaf = theBranch->GetLeaf(name);
  return theLeaf->GetValuePointer();
}

void BNLOpWLSData::SetExEmData(std::string fname) {
  // Check file is there
  struct stat buffer;
  if (stat(fname.c_str(), &buffer) != 0) {
    RAT::info << "BNLOpWLS::SetExEmData: Warning: Could not find Ex/Em data file "
              << "for BNLOpWLS model" << newline;
    return;
  }

  // I assume that the events are stored in such a way that they are
  // monotonically increasing with wavelength
  TFile *f = new TFile(fname.c_str());
  TTree *theTree = (TTree *)f->Get("FluorSpec");
  TBranch *ExBranch = (TBranch *)theTree->GetBranch("Lambda_ex");
  TBranch *EmBranch = (TBranch *)theTree->GetBranch("Wavelength");
  TBranch *IntenBranch = (TBranch *)theTree->GetBranch("Intensity");

  int nEntries = theTree->GetEntries();

  ExEmData.clear();

  std::vector<double> ExWavelength;
  std::vector<double> EmWavelengths;
  std::vector<double> EmIntensities;
  std::vector<std::vector<double>> theData;
  double theIntegral;

  for (int i = 0; i < nEntries; i++) {
    ExWavelength.clear();
    theData.clear();
    theIntegral = 0;

    // I should get these in a different way...
    ExWavelength.push_back(*(double *)(GetPointerToValue(ExBranch, i, ExBranch->GetName())));

    EmWavelengths = *(std::vector<double> *)(GetPointerToValue(EmBranch, i, EmBranch->GetName()));

    EmIntensities = *(std::vector<double> *)(GetPointerToValue(IntenBranch, i, IntenBranch->GetName()));

    // Load the data into slots 1, 2, and 3.
    theData.push_back(ExWavelength);
    theData.push_back(EmWavelengths);

    // Normalize the emission intensities.
    for (size_t j = 0; j < EmIntensities.size(); j++) {
      theIntegral += EmIntensities.at(j);
    }

    for (size_t j = 0; j < EmIntensities.size(); j++) {
      EmIntensities.at(j) = EmIntensities.at(j) / theIntegral;
    }

    theData.push_back(EmIntensities);
    ExEmData.push_back(theData);
  }

  return;
}
