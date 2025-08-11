#ifndef __RAT_FitBonsaiProc__
#define __RAT_FitBonsaiProc__

// #include <Config.hh>

#include <RAT/DS/EV.hh>
#include <RAT/DS/Root.hh>
#include <RAT/FitterInputHandler.hh>
#include <RAT/Processor.hh>
#include <string>

// Load in BONSAI specific libraries
#include <RAT/BONSAI/ariadne.h>
#include <RAT/BONSAI/azimuth_ks.h>
#include <RAT/BONSAI/distpmt.h>
#include <RAT/BONSAI/fourhitgrid.h>
#include <RAT/BONSAI/goodness.h>
#include <RAT/BONSAI/pmt_geometry.h>
#include <RAT/BONSAI/searchgrid.h>

namespace RAT {

class FitBonsaiProc : public RAT::Processor {
 public:
  FitBonsaiProc() : Processor("bonsai"), inputHandler(){};
  virtual ~FitBonsaiProc() {}

  void BeginOfRun(RAT::DS::Run *run);

  virtual RAT::Processor::Result Event(RAT::DS::Root *ds, RAT::DS::EV *ev);

  int nwin(float twin, float *v, int nfit, int *cfit, float *tfit, int *cwin);

  int bs_useCherenkovAngle;
  float bs_nTmin, bs_nTmax;

  RAT::DS::PMTInfo *bs_pmtinfo;

  inline static std::string bs_likeFilename;

  fit_param bspar;
  bonsaifit *bsfit, *cffit;
  pmt_geometry *bsgeom;
  likelihood *bslike;
  goodness *bsgdn;
  fourhitgrid *bsgrid;
  int bs_cables[5000], bs_veto_cables[5000];
  int bs_cables_win[500], bs_veto_cables_win[5000];
  float bs_times[5000], bs_veto_times[5000];
  float bs_charges[5000], bs_veto_charges[5000];
  int bs_n, bs_count;
  int bs_inpmt;
  int bs_hit, bs_nhit, bs_veto_count;
  float bs_bonsai_vtxfit[4];
  double bs_vertex[3];
  float bs_dir[3];
  double bs_ddir[3], bs_wall[3];

  float bs_goodn[2], bs_agoodn;
  float bs_offset = 800.0;
  int bs_nsel;

 protected:
  FitterInputHandler inputHandler;
  // cppflow::model *hitmanModel;
};

}  // namespace RAT

#endif
