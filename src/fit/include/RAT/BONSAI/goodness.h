#ifndef GOODNESS
#define GOODNESS
#include <RAT/BONSAI/bonsaifit.h>
#include <RAT/BONSAI/fit_param.h>
#include <RAT/BONSAI/fitquality.h>
#include <RAT/BONSAI/hitsel.h>
#include <RAT/BONSAI/likelihood.h>
#include <RAT/BONSAI/plato.h>
#include <RAT/BONSAI/searchgrid.h>

// *************************************************************
// * Defines clusfit goodness function and maximization        *
// *************************************************************
class goodness : public fit_param, public fitquality, public hitsel {
  float cosc0, plusdang, minusdang, dirweight;  // direction constraints
  float *ttof, *tau, *hitdir;                   // arrays for time residuals and directions
  float t0, cosc, theta, phi, gdns, gdn0;       // fit results
  float twin, tcent, tnorm;                     // time window, ave. time, time normal.
  int *lists, *buffer, *sort_list, nbranch;     // for sorting
  int ngdn;                                     // number of fit vertices
  dodecahedron dod;                             // defines how surrounding points
  axes orientation;                             // are calculated

  inline void create_lists(int n);  // create hitsel sorting lists and buffer
  void qsort(int *list, int n);     // recursive QuickSort program
  void bubble(void);                // Bubble sort program

 public:
  // construct hit, hitsel and goodness objects from zbs commons
  inline goodness(float r, float z, int ns, float *set, float *pmt_loc, int *bad_ch, int n_raw, int *cable,
                  float *tim_raw, float *chg_raw);
  // construct hit, hitsel and goodness objects with time/charge list
  inline goodness(int ns, float *set, pmt_geometry *geom, int n_raw, int *cable, float *tim_raw, float *chg_raw);
  // construct hit, hitsel and goodness objects with IT structure
  inline goodness(int ns, float *set, pmt_geometry *geom, comtype2 *itevent);
  // destroy goodness object
  inline ~goodness(void);
  // calculate goodness
  virtual float quality(float *vertex);
  // surround a point with test vertices
  inline virtual void check_around(float *vertex, float *result, float radius, float *q, int &max);
  // return number of surrounding test vertices
  inline virtual char ncheck(void);
  // interpolate best fit position
  inline virtual void interpolate(float *vertex, float radius, float *q, float *inter);
  inline virtual int nresult(void);           // return size of result array
  inline virtual void get_result(float *r);   // get result array
  inline virtual void set_result(float *r);   // set result array
  inline virtual void set_branch(int point);  // switch memory to search branch
  // maximizing procedure
  inline void maximize(bonsaifit *fit, searchgrid *grid, bool useAngle = true);
  inline void maximize(bonsaifit *cfit, bonsaifit *bfit, likelihood *bslike, searchgrid *grid, bool useAngle = true);
  inline float get_zero(void);       // get t0
  inline float get_gdn(void);        // get goodness
  inline float get_gdn0(void);       // get goodness
  inline float residual(int hit);    // get time residual
  inline float get_theta(void);      // get zenith angle
  inline float get_phi(void);        // get azimuth angle
  inline float get_cos_theta(void);  // get opening angle
  inline int nfit(void);             // get # of test vertices
};

#include <RAT/BONSAI/goodness.inline>
#endif
