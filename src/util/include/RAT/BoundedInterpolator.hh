#ifndef __RAT_BoundedInterpolator__
#define __RAT_BoundedInterpolator__

#include <Math/Interpolator.h>

#include <vector>

// Wrapper around ROOT::Math::Interpolator class, with extra control of out-of-bound evaluations.
namespace RAT {

class BoundedInterpolator {
 public:
  BoundedInterpolator(const std::vector<double> &x, const std::vector<double> &y,
                      ROOT::Math::Interpolation::Type type = ROOT::Math::Interpolation::kCSPLINE)
      : fInterpolator(x, y, type) {
    bound_left = x.at(0);
    bound_right = x.at(x.size() - 1);
  }

  BoundedInterpolator(unsigned int ndata = 0,
                      ROOT::Math::Interpolation::Type type = ROOT::Math::Interpolation::kCSPLINE)
      : fInterpolator(ndata, type) {}

  bool SetData(const std::vector<double> &x, const std::vector<double> &y) {
    bound_left = x.at(0);
    bound_right = x.at(x.size() - 1);
    return fInterpolator.SetData(x, y);
  }

  bool SetData(unsigned int ndata, const double *x, const double *y) {
    bound_left = x[0];
    bound_right = x[ndata - 1];
    return fInterpolator.SetData(ndata, x, y);
  }

  double Eval(double x) const {
    if (x < bound_left) return fInterpolator.Eval(bound_left + 1e-8);
    if (x > bound_right) return fInterpolator.Eval(bound_right - 1e-8);
    return fInterpolator.Eval(x);
  }

  double Deriv(double x) const {
    if (x < bound_left || x > bound_right) return 0;
    return fInterpolator.Deriv(x);
  }

  double Deriv2(double x) const {
    if (x < bound_left || x > bound_right) return 0;
    return fInterpolator.Deriv2(x);
  }

  double Integ(double xlow, double xhigh) const {
    if (xlow <= bound_left) xlow = bound_left + 1e-8;
    if (xhigh >= bound_right) xhigh = bound_right - 1e-8;
    if (xlow >= xhigh) return 0;
    return fInterpolator.Integ(xlow, xhigh);
  }

 protected:
  ROOT::Math::Interpolator fInterpolator;
  double bound_left, bound_right;
};

}  // namespace RAT

#endif
