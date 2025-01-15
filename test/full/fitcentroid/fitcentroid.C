#include <TFile.h>
#include <TH1.h>
#include <TProfile.h>
#include <TTree.h>

void fitcentroid(std::string event_filename, std::string out_filename) {
  TFile *event_file = new TFile(event_filename.c_str(), "READ");
  TTree *T = (TTree *)event_file->Get("T");
  TFile *out_file = new TFile(out_filename.c_str(), "RECREATE");

  TH1F *hCenterRes =
      new TH1F("hCenterRes", "3-15 MeV electrons at the center;Reconstructed X (mm);Events per bin", 50, -1000, 1000);
  TProfile *hReconVsEnergy = new TProfile(
      "hReconVsEnergy", "Electrons at center (error bars are RMS);Kinetic energy (MeV);Reconstructed X (mm)", 12, 3, 15,
      "s");

  T->Draw("ds.ev.fitResults.fit_position.fX>>hCenterRes", "ds.ev.fitResults.valid_position", "goff");
  T->Draw("ds.ev.fitResults.fit_position.fX:ds.mc.particle.ke>>hReconVsEnergy", "ds.ev.fitResults.valid_position",
          "goff prof");

  hCenterRes->Fit("gaus");
  hReconVsEnergy->SetMinimum(-200);
  hReconVsEnergy->SetMaximum(200);

  out_file->cd();
  hCenterRes->Write();
  hReconVsEnergy->Write();
  event_file->Close();
  out_file->Close();
  delete event_file;
  delete out_file;
}
