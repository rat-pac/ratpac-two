#ifndef __RAT_NPEESTIMATOR__
#define __RAT_NPEESTIMATOR__

#include <TObject.h>

namespace RAT {

/**
 * @brief Estimate the number of PEs in a resolved wave packet using a gaussian single-PE charge PDF.
 * @param charge The measured charge of the wave packet.
 * @param vpe_charge The nominal charge of a single PE.
 * @param npe_estimate_charge_width The width of the Gaussian single-PE charge distribution.
 * @param npe_estimate_max_pes The upper limit for the number of PEs to consider.
 * @return Estimated number of PEs.
 */
size_t EstimateNPE(double charge, double vpe_charge, double npe_estimate_charge_width, size_t npe_estimate_max_pes);

}  // namespace RAT

#endif