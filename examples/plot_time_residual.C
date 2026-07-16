/*****************************************************************************
 * Example analysis macro demonstrating how to use the transit time calculator
 * to compute transit times, and in turn, time  residuals.
 *
 * author: Jieran Shen <jierans@sas.upenn.edu>
 *
 * ***************************************************************************/

#include <TCanvas.h>
#include <TH1D.h>
#include <TLegend.h>

#include <RAT/DSReader.hh>
#include <RAT/Log.hh>
#include <RAT/TransitTimeCalculator.hh>

void plot_time_residual(const std::string& fname) {
  TCanvas* c1 = new TCanvas("c1", "", 800, 600);
  TH1* h_tresid = new TH1D("h_tresid", "Time Residual;Time Residual [ns];Counts", 200, -25, 175);
  TH1* h_time = new TH1D("h_time", "Time;Time [ns];Counts", 200, -25, 175);

  RAT::DSReader dsReader(fname);

  // Constructs the Geant4 Geometry using RATDB and run information stored in the DS file. Required for the G4
  // LightPathCalculator to work properly.
  dsReader.BuildGeometry();

  // Create the transit time calculator instance. This class is cheap to instantiate and should simply be constructed
  // and owned by whoever needs it. Optionally can provide two arguments specifying the type of light path and group
  // velocity calculation should be used. The default here is `g4` and `rindex`, which uses a straight-line traversal in
  // the Geant4 geometry light path calculation, and uses the refractive index to compute the group velocity. See
  // TransitTimeCalculator.hh for more details.
  RAT::TransitTimeCalculator ttc;

  for (size_t iEntry = 0; iEntry < dsReader.GetEntryCount(); ++iEntry) {
    RAT::info << "Processing entry " << iEntry << " of " << (size_t)dsReader.GetEntryCount() << newline;
    const RAT::DS::Root rDS = dsReader.GetEntry(iEntry);
    const RAT::DS::MC* rMC = rDS.GetMC();
    TVector3 mc_pos = rMC->GetMCParticle(0)->GetPosition();
    double mc_time = rMC->GetMCParticle(0)->GetTime();
    for (size_t iEV = 0; iEV < rDS.GetEVCount(); ++iEV) {
      const RAT::DS::EV* rEV = rDS.GetEV(iEV);
      // trigger_time is the time difference between the start time of the simulation and the time that a trigger is
      // issued by the DAQ. Worth noting that all DAQ times (hitPMT, digitPMT, etc) are relative to the trigger time.
      // The trigger time is a MC quantity and is not available in real data.
      double trigger_time = rEV->GetCalibratedTriggerTime();
      for (Int_t pmtid : rEV->GetAllPMTIDs()) {
        const RAT::DS::PMT* current_pmt = rEV->GetPMT(pmtid);
        double hit_time = current_pmt->GetTime();
        TVector3 pmt_pos = dsReader.GetRun().GetPMTInfo()->GetPosition(pmtid);
        RAT::TransitTimeCalculator::Result result = ttc.Compute(mc_pos, pmt_pos);
        // Add back trigger time to get hit time residuals with respect to the start of the particle emission time.
        double tresid = hit_time - result.totalTime + trigger_time - mc_time;
        h_tresid->Fill(tresid);
        h_time->Fill(hit_time);
      }
    }
  }
  h_tresid->SetLineColor(kRed);
  h_tresid->SetStats(0);
  h_tresid->GetXaxis()->SetTitle("Time Residual [ns]");
  h_tresid->GetYaxis()->SetTitle("Counts");
  h_tresid->Draw();

  h_time->SetLineColor(kBlack);
  h_time->Draw("same");

  TLegend* legend = new TLegend(0.7, 0.7, 0.85, 0.85);
  legend->AddEntry(h_tresid, "Time Residual", "l");
  legend->AddEntry(h_time, "Hit Time", "l");
  legend->Draw();

  c1->SetLogy();
  c1->SaveAs("time_residual.pdf");
}
