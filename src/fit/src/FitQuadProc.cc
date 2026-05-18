#include <RAT/DS/FitResult.hh>
#include <RAT/DS/RunStore.hh>
#include <RAT/FitQuadProc.hh>
#include <RAT/FitterInputHandler.hh>
#include <Randomize.hh>
#include <array>

namespace RAT {

FitQuadProc::FitQuadProc() : Processor("quadfitter"), inputHandler() {}

void FitQuadProc::BeginOfRun(DS::Run *run) {
  fRun = run;
  fPMTInfo = run->GetPMTInfo();

  DB *db = DB::Get();
  DBLinkPtr quad_db = db->GetLink("FIT_QUAD");
  fNumQuadPoints = quad_db->GetI("num_points");
  fMaxQuadPoints = quad_db->GetI("max_points");
  fTableCutOff = quad_db->GetI("table_cut_off");
  if (fTableCutOff > fNumPointsTbl.size()) {
    Log::Die("Quad tried to set a table_cut_off larger than the size of fNumPointsTbl.");
  }
  fMaxRadius = quad_db->GetD("max_radius");

  DBLinkPtr table = db->GetLink("FIT_COMMON", "");
  fLightSpeed = table->GetD("light_speed");
  if (fLightSpeed <= 0.0 || fLightSpeed > 299.792458)
    Log::Die("light_speed in FIT_COMMON table must be > 0 and <= 299.792458 mm/ns.");
}

void FitQuadProc::SetS(std::string param, std::string value) {
  if (param == "label") {
    if (value.empty()) throw ParamInvalid(param, "label cannot be empty.");
    fFitLabel = value;
  } else
    throw ParamUnknown(param);
}

void FitQuadProc::SetI(std::string param, int value) {
  if (param == "num_points") {
    fNumQuadPoints = value;
  } else if (param == "max_points") {
    fMaxQuadPoints = value;
  } else if (param == "table_cut_off") {
    fTableCutOff = value;
    if (fTableCutOff > fNumPointsTbl.size())
      throw ParamInvalid(param, "table_cut_off cannot be larger than the size of fNumPointsTbl.");
  } else if (param == "pmt_type") {
    fPMTtype.push_back(value);
  } else
    throw ParamUnknown(param);
}

void FitQuadProc::SetD(std::string param, double value) {
  if (param == "light_speed") {
    if (value <= 0.0 || value > 299.792458)
      throw ParamInvalid(param, "light_speed must be positive and <= 299.792458 mm/ns.");
    fLightSpeed = value;
  } else if (param == "max_radius") {
    if (fMaxX > 0 || fMaxY > 0 || fMaxZ > 0)
      throw ParamInvalid(param, "cannot set max_radius and (max_x or max_y or max_z).");
    fMaxRadius = value;
  } else if (param == "max_x") {
    fMaxX = value;
    fMaxRadius = 0;
  } else if (param == "max_y") {
    fMaxY = value;
    fMaxRadius = 0;
  } else if (param == "max_z") {
    fMaxZ = value;
    fMaxRadius = 0;
  } else
    throw ParamUnknown(param);
}

// Create a table of all the ways to pick 4 numbers out of n
std::vector<std::array<unsigned int, 4>> FitQuadProc::BuildTable(const unsigned int n) {
  if (n > fTableCutOff) Log::Die("Quad: tried to make a table bigger than expected!\n");

  std::vector<std::array<unsigned int, 4>> table;

  std::array<unsigned int, 4> entry;
  for (entry[0] = 0; entry[0] < n; entry[0]++)
    for (entry[1] = entry[0] + 1; entry[1] < n; entry[1]++)
      for (entry[2] = entry[1] + 1; entry[2] < n; entry[2]++)
        for (entry[3] = entry[2] + 1; entry[3] < n; entry[3]++) table.push_back(entry);

  return table;
}

std::array<unsigned int, 4> FitQuadProc::ChoosePMTs(unsigned int nhits) {
  std::array<unsigned int, 4> pmt_ids;
  for (int j = 0; j < 4; j++) {
    while (true) {
      pmt_ids[j] = CLHEP::RandFlat::shootInt(nhits);
      bool same = false;
      for (int k = 0; k < j; k++)
        if (pmt_ids[j] == pmt_ids[k]) same = true;
      if (!same) break;
    }
  }
  return pmt_ids;
}

// Define some helpful inline functions

// Return squared magnitude
static inline double mag2(const double vec[3]) { return vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]; }

// Subtracts the 3-vectors a-b
static inline void vecsub(double *const ans, const double *const a, const double *const b) {
  ans[0] = a[0] - b[0];
  ans[1] = a[1] - b[1];
  ans[2] = a[2] - b[2];
}

// Dot product a.b for 3-vectors
static inline double vecdot(const double *const a, const double *const b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// Invert a 3x3 matrix, "m", returning the answer as "ans". This
// attempts to be highly optimized, minimizing the total number of
// operations by using the explicit solution and avoiding expensive
// divisions as much as possible.
static inline int matinvert(double (*const ans)[3], const double (*const m)[3]) {
  double denominator =
      (m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]) + m[0][1] * (m[1][2] * m[2][0] - m[1][0] * m[2][2]) +
       m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]));

  if (denominator == 0) {
    debug << "Quad.cc::vecdot() Matrix cannot be inverted. Check PMTs selected. Need fixing.\n";
    return -1;
  }
  const double idet = 1. / denominator;

  ans[0][0] = (-m[1][2] * m[2][1] + m[1][1] * m[2][2]) * idet;
  ans[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * idet;
  ans[0][2] = (-m[0][2] * m[1][1] + m[0][1] * m[1][2]) * idet;
  ans[1][0] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) * idet;
  ans[1][1] = (-m[0][2] * m[2][0] + m[0][0] * m[2][2]) * idet;
  ans[1][2] = (m[0][2] * m[1][0] - m[0][0] * m[1][2]) * idet;
  ans[2][0] = (-m[1][1] * m[2][0] + m[1][0] * m[2][1]) * idet;
  ans[2][1] = (m[0][1] * m[2][0] - m[0][0] * m[2][1]) * idet;
  ans[2][2] = (-m[0][1] * m[1][0] + m[0][0] * m[1][1]) * idet;
  return 0;
}

Processor::Result FitQuadProc::Event(DS::Root *ds, DS::EV *ev) {
  if (fMaxRadius > 0 && (fMaxX > 0 || fMaxY > 0 || fMaxZ > 0))
    Log::Die("Quad tried to set both max_radius and (max_x or max_y or max_z).");

  inputHandler.RegisterEvent(ev);

  std::vector<double> pmtx, pmty, pmtz, pmtt;
  for (int pmtid : inputHandler.GetAllHitPMTIDs()) {
    // Select PMTs by type - optional
    if (fPMTtype.size() > 0) {
      int pmtType = fPMTInfo->GetType(pmtid);
      unsigned int iType = 0;
      for (iType = 0; iType < fPMTtype.size(); iType++) {
        if (fPMTtype[iType] == pmtType) break;
      }
      if (iType == fPMTtype.size()) continue;  // No match found
    }

    TVector3 pmtpos = fPMTInfo->GetPosition(pmtid);
    double time = inputHandler.GetTime(pmtid);
    if (time > 1e6) {
      continue;
    }
    pmtt.push_back(time);
    pmtx.push_back(pmtpos.X());
    pmty.push_back(pmtpos.Y());
    pmtz.push_back(pmtpos.Z());
  }
  size_t nhits = pmtt.size();

  DS::FitResult *fit = new DS::FitResult(name, fFitLabel);
  fit->SetValidEnergy(false);
  fit->SetValidDirection(false);
  fit->SetPosition(TVector3(-9999, -9999, -9999));
  fit->SetTime(-9999);

  if (nhits < 4) {
    fit->SetValidTime(false);
    fit->SetValidPosition(false);
    ev->AddFitResult(fit);
    return Processor::Result(FAIL);
  }

  size_t num_pts = nhits <= fTableCutOff ? fNumPointsTbl[nhits] : fNumQuadPoints;
  std::vector<std::array<unsigned int, 4>> pmt_table;
  if (nhits <= fTableCutOff) {
    pmt_table = BuildTable(nhits);
  } else {
    for (size_t i = 0; i < num_pts; i++) {
      pmt_table.push_back(ChoosePMTs(nhits));
    }
  }
  // Arrays for quad points
  std::vector<double> quad_xs, quad_ys, quad_zs, quad_ts;
  for (size_t pt_i = 0; pt_i < num_pts; pt_i++) {
    double min_time = 1e9;
    std::array<unsigned int, 4> pmt_ids = pmt_table[pt_i];
    double pmt_pos[4][3];

    std::array<double, 4> t;
    for (int j = 0; j < 4; j++) {
      t[j] = fLightSpeed * pmtt[pmt_ids[j]];
      if (t[j] < min_time) {
        min_time = t[j];
      }
      pmt_pos[j][0] = pmtx[pmt_ids[j]];
      pmt_pos[j][1] = pmty[pmt_ids[j]];
      pmt_pos[j][2] = pmtz[pmt_ids[j]];
    }

    double rsq[4];
    double N[3], K[3], g[3], h[3], bv[3];
    double M[3][3];
    double iM[3][3] = {};

    // Now do the calculation
    for (int j = 0; j < 4; j++) rsq[j] = mag2(pmt_pos[j]) - t[j] * t[j];

    for (int k = 0; k < 3; k++) {
      M[k][0] = pmt_pos[k + 1][0] - pmt_pos[0][0];
      M[k][1] = pmt_pos[k + 1][1] - pmt_pos[0][1];
      M[k][2] = pmt_pos[k + 1][2] - pmt_pos[0][2];
      N[k] = t[k + 1] - t[0];
      K[k] = (rsq[k + 1] - rsq[0]) / 2;
    }

    // Check if matrix is invertible, if not continue to next point
    if (matinvert(iM, M) != 0) {
      continue;
    }

    for (int w = 0; w < 3; w++) {
      g[w] = iM[w][0] * K[0] + iM[w][1] * K[1] + iM[w][2] * K[2];
      h[w] = iM[w][0] * N[0] + iM[w][1] * N[1] + iM[w][2] * N[2];
    }

    vecsub(bv, pmt_pos[0], g);
    const double a = mag2(h) - 1;
    const double b = -2 * (vecdot(bv, h) - t[0]);
    const double c = mag2(bv) - t[0] * t[0];

    const double s = b * b - 4 * a * c;

    if (s > 0) {
      // Only one root is used. The other represents light
      // propagating backwards in time and is unphysical. I think.
      double tau = (-b + sqrt(s)) / (2 * a);
      if (tau < min_time) {
        double v[3];
        for (int nt = 0; nt < 3; nt++) v[nt] = g[nt] + h[nt] * tau;

        // Maximum distance (mm) for which we will accept a point.
        // This is meant to remove wild fits, not directly
        // restrict them to the physically allowed region.
        // Indeed, if we cut this off at the PMT surface,
        // it would introduce a large bias when we took the
        // median later.  So it should be a little larger than that.
        const double rlimit = fMaxRadius;
        if (mag2(v) < rlimit * rlimit || (abs(v[0]) < fMaxX && abs(v[1]) < fMaxY && abs(v[2]) < fMaxZ)) {
          quad_xs.push_back(v[0]);
          quad_ys.push_back(v[1]);
          quad_zs.push_back(v[2]);
          quad_ts.push_back(tau / fLightSpeed);
          if (quad_xs.size() >= fMaxQuadPoints) break;  // got enough
        }
      }
    }
  }
  size_t quad_pts = quad_xs.size();
  // if (quad_pts < fNumQuadPoints) {
  if (quad_pts < 1) {
    fit->SetValidTime(false);
    fit->SetValidPosition(false);
    ev->AddFitResult(fit);
    return Processor::Result(FAIL);
  }

  std::sort(quad_xs.begin(), quad_xs.end());
  std::sort(quad_ys.begin(), quad_ys.end());
  std::sort(quad_zs.begin(), quad_zs.end());
  std::sort(quad_ts.begin(), quad_ts.end());

  TVector3 best_fit(quad_xs[quad_pts / 2], quad_ys[quad_pts / 2], quad_zs[quad_pts / 2]);

  // Automatically sets SetValidTime(true);
  fit->SetPosition(best_fit);
  fit->SetTime(quad_ts[quad_pts / 2]);
  ev->AddFitResult(fit);
  return Processor::Result(OK);
}

}  // namespace RAT
