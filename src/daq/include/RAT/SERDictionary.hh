////////////////////////////////////////////////////////////////////
/// \class RAT::SERDictionary
///
/// \brief Single-PE response dictionary and per-PE charge scales for the
/// matching-pursuit waveform analyzers
///
/// \details
/// WaveformAnalysisGreedyMP and WaveformAnalysisFSMP both model a waveform as a
/// sparse spike train convolved with a single-PE response, and FSMP seeds its
/// chain from GreedyMP's result. A seed built against a different dictionary
/// than the sampler's is worse than no seed, so the two must describe the SER
/// identically -- which is why that description lives here rather than being
/// duplicated in each analyzer.
///
/// The dictionary is an (nsamples x nsamples*upsample_factor) matrix whose
/// column `c` is the SER delayed to t = c * period / upsample_factor, negative
/// going, scaled so that a column with weight 1 carries exactly VpeCharge() of
/// charge. Columns are cached per distinct template, keyed by everything that
/// can change one.
///
/// Three template types, chosen with "template_type":
///
///   0  lognormal, parameters "lognormal_scale" / "lognormal_shape"
///   1  gaussian, width "gaussian_width", optionally per PMT type through the
///      paired arrays "gaussian_width_pmt_types" / "gaussian_width_pmt_widths"
///   2  whatever PMTPULSE says for this PMT's model -- the same table the
///      simulation generated the pulse from. A "datadriven" table is used
///      directly; an analytic gaussian is *marginalised over its measured width
///      distribution*, which no single width can reproduce; an analytic
///      lognormal uses its mean and width.
///
/// Type 2 is the one to prefer where PMTPULSE describes the detector: it needs
/// no hand-entered widths, it follows the table if the table is refit, and it
/// captures the per-PE width spread that types 0 and 1 average away. For Eos's
/// r7081_hqe that spread is 23% of the mean.
///
/// The per-PE charge scale is similarly per PMT type: "vpe_charge" with
/// optional "vpe_charge_pmt_types"/"vpe_charge_pmt_charges", and the gamma
/// prior on the fitted weight either as the "gamma_k"/"gamma_theta" pair or,
/// per type, as a relative width through "gamma_pmt_types"/"gamma_pmt_rel_sigma".
///
/// With "apply_pulse_width_scale" the template is stretched by ChannelStatus'
/// per-channel pulse_width_scale, the same calibration PMTWaveformGenerator
/// applies when it builds the pulse. Scales are bucketed before they reach the
/// cache key, since one dictionary per channel would not fit in memory.
///
/// Every key beyond the original scalars is optional, so an existing
/// DIGITIZER_ANALYSIS index keeps working unchanged.
///
/// \author Implementation of the Eos FSMP input work; see
///         Notes/Notes.md "Realistic FSMP inputs".
////////////////////////////////////////////////////////////////////
#ifndef __RAT_SERDictionary__
#define __RAT_SERDictionary__

#include <TMatrixD.h>

#include <RAT/DB.hh>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace RAT {

class SERDictionary {
 public:
  /// Template families. Values are the "template_type" ratdb setting.
  enum TemplateType { kLognormal = 0, kGaussian = 1, kPMTPulse = 2 };

  SERDictionary() = default;

  /// Read every template and charge parameter from one DIGITIZER_ANALYSIS
  /// index. `owner` labels log messages and Log::Die() text.
  void Configure(DBLinkPtr tbl, const std::string &owner);

  /// Digitizer geometry to build dictionaries for. Cheap to call per waveform:
  /// it only clears the cache when something actually changed.
  void SetGeometry(int nsamples, double period, double term_ohms, double upsample_factor);

  /// The dictionary this PMT should be fit with, built on first use. Valid
  /// until the next SetGeometry() that changes something.
  const TMatrixD &Get(int pmtid);

  /// Nominal charge of one PE (pC): a column of weight 1 carries this much.
  double VpeCharge(int pmtid) const;

  /// Gamma prior on the fitted weight: mean = k*theta, variance = k*theta^2.
  double GammaK(int pmtid) const;
  double GammaTheta(int pmtid) const;

  /// /rat/procset overrides. False means "not one of mine", so the caller can
  /// carry on looking rather than reporting an unknown parameter.
  bool SetD(const std::string &param, double value);
  bool SetI(const std::string &param, int value);

  int GetTemplateType() const { return fTemplateType; }

 private:
  /// A scalar with optional per-PMT-type overrides, as paired ratdb arrays.
  struct TypeMap {
    double fallback = 0.0;
    std::vector<int> types;
    std::vector<double> values;
    /// The override for this type, or `fallback` if it has none.
    double At(int pmt_type) const;
    /// As At(), but says whether the type was actually listed. Callers that
    /// have a different fallback of their own need to know the difference.
    bool Find(int pmt_type, double &out) const;
  };

  /// One resolved SER shape, in the time convention of RAT::PMTPulse: the
  /// argument is t - (nominal PE time), and the shape integrates to 1 over ns.
  struct Shape {
    enum Kind { kAnalyticLognormal, kAnalyticGaussian, kGaussianMixture, kTabulated };
    Kind kind = kAnalyticGaussian;
    double lognormal_m = 0.0;         ///< "m", the lognormal scale
    double lognormal_s = 0.0;         ///< "sigma", the lognormal shape
    double sigma = 0.0;               ///< gaussian width
    std::vector<double> widths;       ///< kGaussianMixture: component widths
    std::vector<double> width_probs;  ///< and their weights, summing to 1
    std::vector<double> times;        ///< kTabulated, ascending
    std::vector<double> values;       ///< and the shape there, unit integral

    double Evaluate(double dt) const;
  };

  /// Everything that can make two dictionaries differ.
  struct Key {
    int kind = 0;      ///< TemplateType
    int model = -1;    ///< PMTInfo model index, kPMTPulse only
    int width_ps = 0;  ///< gaussian width in ps, kGaussian only
    int scale_bucket = 0;
    int vpe_fc = 0;  ///< vpe_charge in fC
    bool operator<(const Key &o) const {
      return std::tie(kind, model, width_ps, scale_bucket, vpe_fc) <
             std::tie(o.kind, o.model, o.width_ps, o.scale_bucket, o.vpe_fc);
    }
  };

  TypeMap ReadTypeMap(DBLinkPtr tbl, const std::string &scalar, const std::string &types_key,
                      const std::string &values_key) const;

  /// Per-channel width scale, bucketed. Returns 1.0 when the calibration is off.
  double WidthScale(int pmtid) const;
  int ScaleBucket(double scale) const;

  Key KeyFor(int pmtid) const;
  Shape ShapeFor(const Key &key, int pmtid) const;
  /// Read one PMTPULSE model into a Shape, honouring pulse_type/pulse_shape.
  Shape ShapeFromPMTPulse(const std::string &model_name) const;
  void Build(const Key &key, int pmtid, TMatrixD &out) const;

  std::string fOwner;
  int fTemplateType = kLognormal;

  double fLognormalScale = 0.0;
  double fLognormalShape = 0.0;
  TypeMap fGaussianWidth;
  TypeMap fVpeCharge;
  TypeMap fGammaRelSigma;  ///< per-type relative charge width; empty = use k/theta
  double fGammaK = 0.0;
  double fGammaTheta = 0.0;
  bool fApplyWidthScale = false;

  int fNsamples = -1;
  double fPeriod = -1.0;
  double fTermOhms = 0.0;
  double fUpsample = 0.0;

  std::map<Key, TMatrixD> fCache;
  mutable bool fCacheWarned = false;
};

}  // namespace RAT

#endif
