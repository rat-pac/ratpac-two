#include "RAT/WaveformAnalysisL1Deconv.hh"

#include <Math/Functor.h>
#include <Minuit2/Minuit2Minimizer.h>

#include <RAT/Log.hh>
#include <algorithm>
#include <numeric>

#include "RAT/DS/PMTInfo.hh"
#include "RAT/DS/RunStore.hh"
#include "RAT/DS/WaveformAnalysisResult.hh"
#include "RAT/WaveformUtil.hh"

namespace RAT {

void WaveformAnalysisL1Deconv::Configure(const std::string& config_name) {
  try {
    fDigit = DB::Get()->GetLink("DIGITIZER_ANALYSIS", config_name);
    fTemplateDelay = fDigit->GetD("template_delay");
    fPMTPulseShapeTimes = fDigit->GetDArray("template_samples");
    fPMTPulseShapeValues = fDigit->GetDArray("template_values");
    fUpsampleFactor = fDigit->GetD("upsample_factor");
    fNoiseSigma = fDigit->GetD("noise_sigma");
    fPeakThreshold = fDigit->GetD("peak_threshold");
  } catch (DBNotFoundError&) {
    RAT::Log::Die("WaveformAnalysisL1Deconv: Unable to find analysis parameters.");
  }
  // Precompute the template spline
  templateSpline = std::make_unique<TSpline3>("template", fPMTPulseShapeTimes.data(), fPMTPulseShapeValues.data(),
                                              fPMTPulseShapeTimes.size());
}

void WaveformAnalysisL1Deconv::SetD(std::string param, double value) {
  if (param == "template_delay") {
    fTemplateDelay = value;
  } else if (param == "upsample_factor") {
    fUpsampleFactor = value;
  } else if (param == "noise_sigma") {
    fNoiseSigma = value;
  } else if (param == "peak_threshold") {
    fPeakThreshold = value;
  } else {
    throw Processor::ParamUnknown(param);
  }
}

void WaveformAnalysisL1Deconv::DoAnalysis(DS::DigitPMT* digitpmt, const std::vector<UShort_t>& digitWfm) {
  double pedestal = digitpmt->GetPedestal();
  if (pedestal == -9999) {
    RAT::Log::Die("WaveformAnalysisL1Deconv: Pedestal is invalid! Did you run WaveformPrep first?");
  }
  // Convert from ADC to mV
  std::vector<double> voltWfm = WaveformUtil::ADCtoVoltage(digitWfm, fVoltageRes, pedestal);

  // Compute the S matrix if it hasn't been computed yet
  if (!sMatrixComputed) {
    const size_t nsamples = voltWfm.size();
    const size_t nupsampled = nsamples * fUpsampleFactor;
    sMatrix.resize(nsamples, nupsampled);
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (size_t i = 0; i < nsamples; ++i) {
      for (size_t j = 0; j < nupsampled; ++j) {
        double shifted_index = i - (static_cast<double>(j) / fUpsampleFactor - fTemplateDelay);
        if (shifted_index >= 0.0 && shifted_index < static_cast<double>(nsamples)) {
          sMatrix(i, j) = templateSpline->Eval(shifted_index);
        } else {
          sMatrix(i, j) = 0.0;
        }
      }
    }
    sMatrixComputed = true;
  }

  // Perform L1 deconvolution
  Eigen::VectorXd voltWfmVec = Eigen::Map<Eigen::VectorXd>(voltWfm.data(), voltWfm.size());
  Eigen::VectorXd result = L1Deconvolution(voltWfmVec, sMatrix, fNoiseSigma);

  // Find the peaks in the deconvolved signal
  std::vector<double> peaks;
  for (int i = 1; i < result.size() - 1; ++i) {
    if (result[i] > result[i - 1] && result[i] > result[i + 1] && result[i] > fPeakThreshold) {
      peaks.push_back(i);
    }
  }

  // Store the results
  DS::WaveformAnalysisResult* fit_result = digitpmt->GetOrCreateWaveformAnalysisResult("L1Deconv");
  for (const auto& peak : peaks) {
    double peak_time = (static_cast<double>(peak) / fUpsampleFactor - fTemplateDelay) * fTimeStep;
    fit_result->AddPE(peak_time, 1, {{"peak_value", result[peak]}});
  }
}

Eigen::VectorXd WaveformAnalysisL1Deconv::L1Deconvolution(const Eigen::VectorXd& voltWfmVec,
                                                          const Eigen::MatrixXd& sMatrix, double noiseSigma) {
  // Validate input dimensions
  if (sMatrix.rows() != voltWfmVec.size() || sMatrix.rows() == 0 || sMatrix.cols() == 0) {
    RAT::Log::Die("WaveformAnalysisL1Deconv: Invalid matrix dimensions");
  }

  const size_t m = sMatrix.rows();  // Number of observations
  const size_t n = sMatrix.cols();  // Number of variables

  // Size checks
  if (m > 1e6 || n > 1e6) {
    RAT::Log::Die("WaveformAnalysisL1Deconv: Matrix dimensions too large");
  }

  // Calculate lambda
  const double lambda = std::max(1e-6, noiseSigma * std::sqrt(2.0 * std::log(static_cast<double>(n))));

  // Scale matrix for numerical stability
  const double scale = std::max(1e-10, sMatrix.norm());
  Eigen::MatrixXd scaledMatrix = sMatrix / scale;
  Eigen::VectorXd scaledVec = voltWfmVec / scale;

  // Create augmented matrix [S; λI]
  Eigen::SparseMatrix<double> A(m + n, n);
  std::vector<Eigen::Triplet<double>> triplets;
  triplets.reserve(m * n + n);  // Reserve space for S and I

  // Add S matrix elements
  for (size_t i = 0; i < m; ++i) {
    for (size_t j = 0; j < n; ++j) {
      if (std::abs(scaledMatrix(i, j)) > 1e-6) {
        triplets.emplace_back(i, j, scaledMatrix(i, j));
      }
    }
  }

  // Add λI matrix elements
  for (size_t i = 0; i < n; ++i) {
    triplets.emplace_back(m + i, i, lambda);
  }

  // Set matrix from triplets
  A.setFromTriplets(triplets.begin(), triplets.end());
  A.makeCompressed();

  // Create augmented vector [y; 0]
  Eigen::VectorXd b(m + n);
  b.head(m) = scaledVec;
  b.tail(n).setZero();

  // Solve using QR decomposition
  Eigen::SparseQR<Eigen::SparseMatrix<double>, Eigen::COLAMDOrdering<int>> solver;
  solver.compute(A);

  if (solver.info() != Eigen::Success) {
    RAT::Log::Die("WaveformAnalysisL1Deconv: Matrix decomposition failed");
  }

  Eigen::VectorXd x = solver.solve(b);

  if (solver.info() != Eigen::Success) {
    RAT::Log::Die("WaveformAnalysisL1Deconv: Solver failed");
  }

  // Rescale solution
  return x * scale;
}

}  // namespace RAT