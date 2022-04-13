#include <RAT/DS/MC.hh>
#include <RAT/DS/Root.hh>
#include <RAT/DS/MCTrack.hh>
#include <RAT/DS/MCTrackStep.hh>

#include <TFile.h>
#include <TTree.h>

#include <iostream>
#include <string>

using namespace RAT::DS;
using namespace std;

int main(int argc, char** argv)
{
  if ( argc < 1 )
    return 1;
  string infile = argv[1];

  TFile* tfile = new TFile(infile.c_str());
  TTree* tree = (TTree*)tfile->Get("T");

  Root* ds = new Root();
  tree->SetBranchAddress("ds", &ds);

  int escapedGamma = 0;
  int escapedElectron = 0;
  int other = 0;

  for(int i=0; i<tree->GetEntries(); i++){
    tree->GetEvent(i);
    MC* mc = ds->GetMC();
    int trackCount = mc->GetMCTrackCount();
    bool eGamma = false;
    bool eElectron = false;
    bool eOther = false;
    for( int trackid=0; trackid<trackCount; trackid++ ){
      MCTrack* track = mc->GetMCTrack(trackid);
      string pname = track->GetParticleName();
      for(int stepid=0; stepid < track->GetMCTrackStepCount(); stepid++){
        MCTrackStep* step = track->GetMCTrackStep(stepid);
        string volume = step->GetVolume();
        if( volume == "eos_inner" ){
          if( pname == "e-" ){
            eElectron = true;
          }
          else if( pname == "gamma" ){
            eGamma = true;
          } else {
            eOther = true;
            cout << eOther << endl;
          }
        }
      }
      escapedGamma += eGamma ? 1 : 0;
      escapedElectron += eElectron ? 1 : 0;
      other += eOther ? 1 : 0;
    }
  }

  cout << "Gamma " << escapedGamma << endl;
  cout << "Electron " << escapedElectron << endl;
  cout << "Other " << other << endl;
}
