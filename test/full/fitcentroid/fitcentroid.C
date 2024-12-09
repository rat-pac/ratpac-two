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

  T->Draw("ds.ev.fitResults.fit_position.fX>>hCenterRes",
          "ds.mc.particle.ke < 0.0205 && ds.mc.particle.pos.fX < 0.05 && ds.ev.fitResults.valid_position", "goff");
  T->Draw("ds.ev.fitResults.fit_position.fX-ds.mc.particle.pos.fX:(ds.mc.particle.pos.Mag()/437)>>hReconVsRadius",
          "ds.mc.particle.ke < 0.0205 && ds.mc.particle.pos.fX > 0.05 && ds.ev.fitResults.valid_position", "goff prof");
  T->Draw("ds.ev.fitResults.fit_position.fX:ds.mc.particle.ke>>hReconVsEnergy",
          "ds.mc.particle.ke > 0.0205 && ds.mc.particle.pos.fX < 0.05 && ds.ev.fitResults.valid_position", "goff prof");

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
