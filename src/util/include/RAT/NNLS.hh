#ifndef __RAT_NNLS__
#define __RAT_NNLS__

#include <vector>

#include "TMatrixD.h"
#include "TVectorD.h"

namespace RAT {
namespace Math {

/**
 * Lawson–Hanson Non-Negative Least Squares (NNLS).
 * Solve   min_x ||A x - b||_2  subject to x >= 0.
 *
 * @param A  (m x n) design matrix
 * @param b  (m)     RHS vector
 * @param tol Numerical tolerance for optimality (default ~ 1e-12 * ||A^T b||_inf)
 * @param max_outer Optional cap on outer iterations (0 => default 3*n)
 * @return x  solution vector (size n). Throws std::runtime_error on hard failure.
 */
TVectorD NNLS_LawsonHanson(const TMatrixD& A, const TVectorD& b, double tol = -1.0, int max_outer = 0);

/** Convenience: return residual norm ||A x - b||_2 after solve. */
double NNLS_ResidualNorm(const TMatrixD& A, const TVectorD& b, const TVectorD& x);

}  // namespace Math
}  // namespace RAT

#endif
