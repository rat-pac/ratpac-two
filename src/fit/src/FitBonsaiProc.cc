#include <RAT/DB.hh>
#include <RAT/DS/EV.hh>
#include <RAT/DS/FitResult.hh>
#include <RAT/DS/PMTInfo.hh>
#include <RAT/DS/Root.hh>
#include <RAT/DS/RunStore.hh>
#include <RAT/FitBonsaiProc.hh>
#include <RAT/Log.hh>
#include <RAT/Processor.hh>
#include <RAT/Rat.hh>
#include <filesystem>

// Imported from FRED
#include <RAT/DS/PMT.hh>
#include <RAT/DS/Run.hh>

// Load in BONSAI specific libraries
#include <RAT/BONSAI/ariadne.h>
#include <RAT/BONSAI/azimuth_ks.h>
#include <RAT/BONSAI/distpmt.h>
#include <RAT/BONSAI/fourhitgrid.h>
#include <RAT/BONSAI/goodness.h>
#include <RAT/BONSAI/hits.h>
#include <RAT/BONSAI/pmt_geometry.h>
#include <RAT/BONSAI/searchgrid.h>

namespace RAT {

FitBonsaiProc::FitBonsaiProc() : RAT::Processor("bonsai") {
  RAT::DB *db = RAT::DB::Get();
  printf("FitBonsaiProc: Initialisation of the BONSAI processor.\n");
}

void FitBonsaiProc::BeginOfRun(RAT::DS::Run *run) {
  RAT::DB *db = RAT::DB::Get();
  RAT::DBLinkPtr bonsai_ptr = db->GetLink("BONSAI");

  bs_likeFilename = bonsai_ptr->GetS("likelihoodFileName");

  bs_useCherenkovAngle = bonsai_ptr->GetI("useCherenkovAngle");
  bs_nTmin = bonsai_ptr->GetD("nTmin");
  bs_nTmax = bonsai_ptr->GetD("nTmax");
  hits::NS_TO_CM = bonsai_ptr->GetD("mediaSpeedOfLight");
  hits::CM_TO_NS = (float)(1. / hits::NS_TO_CM);
  hits::CM_TO_NS2 = (float)1. / (hits::NS_TO_CM * hits::NS_TO_CM);

  // NS_TO_CM3 = (float) bonsai_ptr->GetD("mediaSpeedOfLight");

  printf("FitBonsaiProc: Loading useCherenkovAngle -> %d\n", bs_useCherenkovAngle);
  printf("FitBonsaiProc: Loading nTmin -> %3.1f\n", bs_nTmin);
  printf("FitBonsaiProc: Loading nTmax -> %3.1f\n", bs_nTmax);
  printf("FitBonsaiProc: Resulting in a n%d ns time window\n", int(bs_nTmax - bs_nTmin));
  printf("FitBonsaiProc: Loading Group Media Light Speed -> %3.1f ns/cm\n", hits::NS_TO_CM);
  printf("FitBonsaiProc: User requested Likelihood filename -> %s \n", bs_likeFilename.c_str());

  bs_pmtinfo = run->GetPMTInfo();
  bs_inpmt = run->GetPMTInfo()->GetPMTCount();
  float xyz[3 * bs_inpmt + 1];

  for (bs_hit = 0; bs_hit < bs_inpmt; bs_hit++) {
    TVector3 pos = bs_pmtinfo->GetPosition(bs_hit);
    xyz[3 * bs_hit] = pos[0] * 0.1;
    xyz[3 * bs_hit + 1] = pos[1] * 0.1;
    xyz[3 * bs_hit + 2] = pos[2] * 0.1;
    // printf("Loading the PMT array: %d %f %f %f \n",bs_hit,pos[0] * 0.1,pos[1] * 0.1,pos[2] * 0.1);
  }
  bsgeom = new pmt_geometry(bs_inpmt, xyz);
  bslike = new likelihood(bsgeom->cylinder_radius(), bsgeom->cylinder_height());
  bsfit = new bonsaifit(bslike);
}

int FitBonsaiProc::nwin(float twin, float *v, int nfit, int *cfit, float *tfit, int *cwin) {
  if (nfit <= 0) return (0);

  float ttof[nfit], tsort[nfit], dx, dy, dz;
  int hit, nwin = 0, nwindow, hstart_test, hstart, hstop;

  // calculate t-tof for each hit
  for (hit = 0; hit < nfit; hit++) {
    TVector3 pos = bs_pmtinfo->GetPosition(cfit[hit] - 1);
    dx = pos.X() * 0.1 - v[0];
    dy = pos.Y() * 0.1 - v[1];
    dz = pos.Z() * 0.1 - v[2];
    tsort[hit] = ttof[hit] = tfit[hit] - sqrt(dx * dx + dy * dy + dz * dz) * hits::CM_TO_NS;
  }
  std::sort(tsort, tsort + nfit);

  // find the largest number of hits in a time window <= twin
  nwindow = 1;
  hstart_test = hstart = 0;
  while (hstart_test < nfit - nwindow) {
    hstop = hstart_test + nwindow;
    while ((hstop < nfit) && (tsort[hstop] - tsort[hstart_test] <= twin)) {
      hstart = hstart_test;
      nwindow++;
      hstop++;
    }
    hstart_test++;
  }
  hstop = hstart + nwindow - 1;
  for (hit = 0; hit < nfit; hit++) {
    if (ttof[hit] < tsort[hstart]) continue;
    if (ttof[hit] > tsort[hstop]) continue;
    cwin[nwin++] = cfit[hit];
  }
  if (nwin != nwindow) printf("nwin error %d!=%d\n", nwin, nwindow);
  return (nwindow);
}

RAT::Processor::Result FitBonsaiProc::Event(RAT::DS::Root *ds, RAT::DS::EV *ev) {
  RAT::DS::FitResult *fit = new RAT::DS::FitResult("Bonsai");

  // Perform the fit
  bs_nhit = ev->GetPMTCount();
  int bs_hit = 0;
  for (int bs_hitID : ev->GetAllPMTIDs()) {  // New way of doing things
    // RAT::DS::PMT *pmt = ev->GetOrCreatePMT(pmtc);
    // for (bs_hit = 0; bs_hit < bs_nhit; bs_hit++) { // Old way of doing things
    //  Analogue option, will need switch for digital output
    bs_cables[bs_hit] = ev->GetOrCreatePMT(bs_hitID)->GetID() + 1;
    bs_times[bs_hit] = ev->GetOrCreatePMT(bs_hitID)->GetTime() + bs_offset;
    bs_charges[bs_hit] = ev->GetOrCreatePMT(bs_hitID)->GetCharge();
    // printf("PMT %d time: %f ns\n",bs_cables[bs_hit],ev->GetOrCreatePMT(bs_hit)->GetTime());
    bs_hit += 1;
  }
  bsgdn = new goodness(bslike->sets(), bslike->chargebins(), bsgeom, bs_nhit, bs_cables, bs_times, bs_charges);
  bs_nsel = bsgdn->nselected();

  if (bs_nsel < 4) {
    TVector3 fitPosition(0.0, 0.0, 0.0);
    TVector3 fitDirection(1.0, 0.0, 0.0);
    double fitEnergy = -1.0;
    double fitTime = 0.0;

    fit->SetPosition(fitPosition);
    fit->SetDirection(fitDirection);
    fit->SetEnergy(fitEnergy);
    fit->SetTime(fitTime);
    fit->SetIntFigureOfMerit("nT", -1);
    fit->SetIntFigureOfMerit("n100", -1);
    fit->SetIntFigureOfMerit("n400", -1);
    fit->SetIntFigureOfMerit("nOff", -1);

    fit->SetDoubleFigureOfMerit("positionGoodness", -1.0);
    fit->SetDoubleFigureOfMerit("closestPMT", -1.0);
    fit->SetDoubleFigureOfMerit("directionGoodness", -1.0);
    fit->SetDoubleFigureOfMerit("azimuthKS", -1.0);
    fit->SetDoubleFigureOfMerit("distpmt", -1.0);

    ev->AddFitResult(fit);

    delete bsgdn;
    bslike->set_hits(NULL);
    return RAT::Processor::OK;

  } else {
    bsgrid = new fourhitgrid(bsgeom->cylinder_radius(), bsgeom->cylinder_height(), bsgdn);
    bslike->set_hits(bsgdn);

    bool use_cherenkov_angle = true;
    if (bs_useCherenkovAngle == 0) use_cherenkov_angle = false;
    bslike->maximize(bsfit, bsgrid, use_cherenkov_angle);  // use_cherenkov_angle);

    *bs_bonsai_vtxfit = bsfit->xfit();
    bs_bonsai_vtxfit[1] = bsfit->yfit();
    bs_bonsai_vtxfit[2] = bsfit->zfit();
    int nT = bslike->nwind(bs_bonsai_vtxfit, (float)bs_nTmin, (float)bs_nTmax);
    bslike->ntgood(bs_bonsai_vtxfit, 0, bs_goodn[0]);

    TVector3 fitPosition(10. * bsfit->xfit(), 10. * bsfit->yfit(), 10. * bsfit->zfit());
    //     TVector3 fitDirection(1.0, 0.0, 0.0);
    double fitEnergy = -1.0;
    double fitTime = bslike->get_zero() - bs_offset;

    // Do direction fit
    int nTwin = nwin(float(bs_nTmax - bs_nTmin), bs_bonsai_vtxfit, bs_nhit, bs_cables, bs_times, bs_cables_win);
    // int nTwin=nwin(9,bs_bonsai_vtxfit,bs_nhit,bs_cables,bs_times,bs_cables_win);

    float apmt[3 * nTwin];
    // fill PMT positions into an array
    for (int bs_hit = 0; bs_hit < nTwin; bs_hit++) {
      TVector3 nTpos = bs_pmtinfo->GetPosition(bs_cables_win[bs_hit] - 1);
      apmt[3 * bs_hit] = nTpos.X() * 0.1;
      apmt[3 * bs_hit + 1] = nTpos.Y() * 0.1;
      apmt[3 * bs_hit + 2] = nTpos.Z() * 0.1;
    }
    // call direction fit and save results
    bs_dir[0] = bs_dir[1] = bs_dir[2] = -2;

    ariadne ari(bs_bonsai_vtxfit, nTwin, apmt, 0.719);
    ari.fit();
    bs_agoodn = ari.dir_goodness();
    if (bs_agoodn >= 0) ari.dir(bs_dir);
    // dir_goodness = bs_agoodn;
    TVector3 fitDirection(bs_dir[0], bs_dir[1], bs_dir[2]);

    fit->SetPosition(fitPosition);
    fit->SetDirection(fitDirection);
    fit->SetEnergy(fitEnergy);
    fit->SetTime(fitTime);
    fit->SetIntFigureOfMerit("nT", nT);
    fit->SetIntFigureOfMerit("n100", bslike->nwind(bs_bonsai_vtxfit, -10, 90));
    fit->SetIntFigureOfMerit("n400", bslike->nwind(bs_bonsai_vtxfit, -10, 390));
    fit->SetIntFigureOfMerit("nOff", bslike->nwind(bs_bonsai_vtxfit, -150, -50));
    fit->SetDoubleFigureOfMerit("positionGoodness", bs_goodn[0]);
    float r2pmt = bsgeom->cylinder_radius() - sqrt(bsfit->xfit() * bsfit->xfit() + bsfit->yfit() * bsfit->yfit());
    float z2pmt = bsgeom->cylinder_height() - sqrt(bsfit->zfit() * bsfit->zfit());
    fit->SetDoubleFigureOfMerit("closestPMT", (r2pmt < z2pmt) ? r2pmt * 10. : z2pmt * 10.);
    fit->SetDoubleFigureOfMerit("directionGoodness", bs_agoodn);
    fit->SetDoubleFigureOfMerit("azimuthKS", azimuth_ks(nTwin, apmt, bs_bonsai_vtxfit, bs_dir));
    bs_vertex[0] = (double)bs_bonsai_vtxfit[0];
    bs_vertex[1] = (double)bs_bonsai_vtxfit[1];
    bs_vertex[2] = (double)bs_bonsai_vtxfit[2];
    bs_ddir[0] = (double)bs_dir[0];
    bs_ddir[1] = (double)bs_dir[1];
    bs_ddir[2] = (double)bs_dir[2];
    fit->SetDoubleFigureOfMerit(
        "distpmt", distpmt(bs_vertex, bs_ddir, bsgeom->cylinder_radius(), bsgeom->cylinder_height(), bs_wall));
    ev->AddFitResult(fit);

    delete bsgrid;
    delete bsgdn;
    bslike->set_hits(NULL);

    return RAT::Processor::OK;
  }
}

}  // namespace RAT
