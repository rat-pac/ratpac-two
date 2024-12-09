// void make_plots(TFile *event_file, TTree *T, TFile *out_file)
// {
//   TH1F *hCenterRes = new TH1F("hCenterRes", "20 keVee electrons at the center;Reconstructed X (mm);Events per bin",
//   40, -800, 800); TProfile *hReconVsRadius = new TProfile("hReconVsRadius", "20 keVee electrons (error bars are
//   RMS);(Radius/437 mm)**3;Reconstructed X - True X (mm)", 5, 0, 1, "s"); TProfile *hReconVsEnergy = new
//   TProfile("hReconVsEnergy", "Electrons at center (error bars are RMS);Kinetic energy (keVee);Reconstructed X (mm)",
//   5, 20, 100,"s");
//
//   T->Draw("ev.centroid.pos.fX>>hCenterRes","mc.particle.ke < 0.0205 && mc.particle.pos.fX < 0.05","goff");
//   T->Draw("ev.centroid.pos.fX-mc.particle.pos.fX:(mc.particle.pos.Mag()/437)>>hReconVsRadius","mc.particle.ke <
//   0.0205 && mc.particle.pos.fX > 0.05","goff prof");
//   T->Draw("ev.centroid.pos.fX:mc.particle.ke*1000>>hReconVsEnergy","mc.particle.ke > 0.0205 && mc.particle.pos.fX <
//   0.05","goff prof");
//
//   hCenterRes->Fit("gaus");
//   hReconVsRadius->SetMinimum(-200);
//   hReconVsRadius->SetMaximum(200);
//   hReconVsEnergy->SetMinimum(-200);
//   hReconVsEnergy->SetMaximum(200);
//
//   out_file->cd();
//   hCenterRes->Write();
//   hReconVsRadius->Write();
//   hReconVsEnergy->Write();
// }

void fitcentroid(std::string event_filename, std::string out_filename) {
  TFile *event_file = new TFile(event_filename.c_str(), "READ");
  TTree *T = (TTree *)event_file->Get("T");
  TFile *out_file = new TFile(out_filename.c_str(), "RECREATE");

  TH1F *hCenterRes =
      new TH1F("hCenterRes", "20 keVee electrons at the center;Reconstructed X (mm);Events per bin", 50, -1000, 1000);
  TProfile *hReconVsRadius = new TProfile(
      "hReconVsRadius", "20 keVee electrons (error bars are RMS);(Radius/437 mm)**3;Reconstructed X - True X (mm)", 5,
      0, 1, "s");
  TProfile *hReconVsEnergy =
      new TProfile("hReconVsEnergy",
                   "Electrons at center (error bars are RMS);Kinetic energy (MeV);Reconstructed X (mm)", 5, 3, 15, "s");

  T->Draw("ds.ev.fitResults.fit_position.fX>>hCenterRes", "ds.mc.particle.ke < 0.0205 && ds.mc.particle.pos.fX < 0.05",
          "goff");
  T->Draw("ds.ev.fitResults.fit_position.fX-ds.mc.particle.pos.fX:(ds.mc.particle.pos.Mag()/437)>>hReconVsRadius",
          "ds.mc.particle.ke < 0.0205 && ds.mc.particle.pos.fX > 0.05", "goff prof");
  T->Draw("ds.ev.fitResults.fit_position.fX:ds.mc.particle.ke>>hReconVsEnergy",
          "ds.mc.particle.ke > 0.0205 && ds.mc.particle.pos.fX < 0.05", "goff prof");

  hCenterRes->Fit("gaus");
  hReconVsRadius->SetMinimum(-2000);
  hReconVsRadius->SetMaximum(2000);
  hReconVsEnergy->SetMinimum(-200);
  hReconVsEnergy->SetMaximum(200);

  out_file->cd();
  hCenterRes->Write();
  hReconVsRadius->Write();
  hReconVsEnergy->Write();
  event_file->Close();
  out_file->Close();
  delete event_file;
  delete out_file;
}
