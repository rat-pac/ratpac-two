#ifndef __RAT_ROOTINTERPOLATOR_HH__
#define __RAT_ROOTINTERPOLATOR_HH__

#include <TGraph.h>

#include <RAT/Log.hh>

namespace RAT {

// A Interpolator class based on ROOT's TGraph. Adds more control over out-of-domain behaviors
class ROOTInterpolator {
 public:
  enum class kind_t {
    kLinear,
    kCubic,
  };
  enum class extrapolation_t {
    kError,        // no interpolation, throws an error instead
    kExtrapolate,  // extrapolate. Default behavior of TGraph::Eval
    kClamp,        // clamp to the nearest point
    kConstant,     // constant value
  };
  ROOTInterpolator() : fGraph(nullptr) {}
  ROOTInterpolator(const TGraph& g, kind_t kind = kind_t::kLinear,
                   extrapolation_t extrapolation = extrapolation_t::kExtrapolate, double fillvalue = 0.0)
      : fGraph(std::make_unique<TGraph>(g)), fKind(kind), fExtrapolation(extrapolation), fFillValue(fillvalue) {
    prepareGraph();
  }
  ROOTInterpolator(TGraph&& graph, kind_t kind = kind_t::kLinear,
                   extrapolation_t extrapolation = extrapolation_t::kExtrapolate, double fillvalue = 0.0) noexcept
      : fGraph(std::make_unique<TGraph>(std::move(graph))),
        fKind(kind),
        fExtrapolation(extrapolation),
        fFillValue(fillvalue) {
    prepareGraph();
  }
  ROOTInterpolator& operator=(const ROOTInterpolator& other) {
    if (this != &other) {
      fGraph = std::make_unique<TGraph>(*other.fGraph);
      fMinX = other.fMinX;
      fMaxX = other.fMaxX;
      fKind = other.fKind;
      fExtrapolation = other.fExtrapolation;
      fFillValue = other.fFillValue;
    }
    return *this;
  }
  ROOTInterpolator& operator=(ROOTInterpolator&& other) noexcept {
    if (this != &other) {
      fGraph = std::move(other.fGraph);
      fMinX = other.fMinX;
      fMaxX = other.fMaxX;
      fKind = other.fKind;
      fExtrapolation = other.fExtrapolation;
      fFillValue = other.fFillValue;
    }
    return *this;
  }

  ROOTInterpolator(const std::vector<double>& x, const std::vector<double>& y, kind_t kind = kind_t::kLinear,
                   extrapolation_t extrapolation = extrapolation_t::kExtrapolate, double fillvalue = 0.0)
      : fGraph(std::make_unique<TGraph>(x.size(), x.data(), y.data())),
        fKind(kind),
        fExtrapolation(extrapolation),
        fFillValue(fillvalue) {
    prepareGraph();
  }

  void SetGraph(const TGraph& g) {
    fGraph = std::make_unique<TGraph>(g);
    prepareGraph();
  }
  void SetGraph(TGraph&& graph) noexcept {
    fGraph = std::make_unique<TGraph>(std::move(graph));
    prepareGraph();
  }
  void SetData(const std::vector<double>& x, const std::vector<double>& y) {
    fGraph = std::make_unique<TGraph>(x.size(), x.data(), y.data());
    prepareGraph();
  }

  void SetFillValue(double fillValue) { fFillValue = fillValue; }
  void SetKind(kind_t kind) { fKind = kind; }
  void SetExtrapolation(extrapolation_t extrapolation) { fExtrapolation = extrapolation; }
  double GetFillValue() const { return fFillValue; }
  kind_t GetKind() const { return fKind; }
  extrapolation_t GetExtrapolation() const { return fExtrapolation; }
  const TGraph* GetGraph() const { return fGraph.get(); }

  double operator()(double xx) const {
    if (!fGraph) {
      Log::Die("Interpolator has no graph set.");
    }
    double result;
    if (xx > fMinX && xx < fMaxX) {
      switch (fKind) {
        case kind_t::kLinear:
          result = fGraph->Eval(xx);
          break;
        case kind_t::kCubic:
          result = fGraph->Eval(xx, 0, "S");
          break;
      }
    } else {  // out of bound
      switch (fExtrapolation) {
        case extrapolation_t::kError:
          Log::Die("Interpolator called with out-of-bound value: %f", xx);
          break;
        case extrapolation_t::kExtrapolate:
          return fGraph->Eval(xx);
          break;
        case extrapolation_t::kClamp:
          if (xx < fMinX) {
            result = fGraph->Eval(fMinX);
          } else {
            result = fGraph->Eval(fMaxX);
          }
          break;
        case extrapolation_t::kConstant:
          result = fFillValue;
          break;
      }
    }
    return result;
  }

 protected:
  std::unique_ptr<TGraph> fGraph;
  double fMinX, fMaxX;
  kind_t fKind = kind_t::kLinear;
  extrapolation_t fExtrapolation = extrapolation_t::kExtrapolate;
  double fFillValue = 0.0;
  void prepareGraph() {
    if (!fGraph) {
      Log::Die("Interpolator has no graph set.");
    }
    fGraph->Sort();
    fMinX = fGraph->GetX()[0];
    fMaxX = fGraph->GetX()[fGraph->GetN() - 1];
  }
};

}  // namespace RAT

#endif  // __RAT_ROOTINTERPOLATOR_HH__
