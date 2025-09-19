#include <RAT/NNLS.hh>
#include <algorithm>
#include <stdexcept>

#include "TDecompQRH.h"
#include "TDecompSVD.h"

namespace RAT {
namespace Math {

namespace {
// Build A[:, P] as a dense matrix with columns in 'Pidx'.
// Reuse existing matrix when possible to avoid repeated allocations.
static void ColSubset(const TMatrixD& A, const std::vector<int>& Pidx, TMatrixD& AP) {
  const int m = A.GetNrows();
  const int k = static_cast<int>(Pidx.size());
  if (AP.GetNrows() != m || AP.GetNcols() != k) {
    AP.ResizeTo(m, k);
  }
  for (int j = 0; j < k; ++j) {
    const int col = Pidx[j];
    for (int i = 0; i < m; ++i) AP(i, j) = A(i, col);
  }
}

// Solve least squares AP * z ~= b using QR decomposition with SVD fallback.
static TVectorD SolveLeastSquares(const TMatrixD& AP, const TVectorD& b) {
  const int ncols = AP.GetNcols();
  const int nrows = AP.GetNrows();

  TVectorD z(ncols);
  if (ncols == 0) {
    z.Zero();
    return z;
  }

  // Use SVD for underdetermined systems
  if (nrows < ncols) {
    TDecompSVD svd(AP);
    Bool_t ok = kFALSE;
    TVectorD temp_z = svd.Solve(b, ok);
    if (!ok) throw std::runtime_error("NNLS: least-squares solve failed (SVD for underdetermined).");

    // Manual copy for numerical stability
    z.ResizeTo(temp_z.GetNrows());
    for (int i = 0; i < temp_z.GetNrows(); ++i) {
      z[i] = temp_z[i];
    }
    return z;
  }

  // Try QR decomposition first, fallback to SVD if needed
  TDecompQRH qr(AP);
  Bool_t ok = kFALSE;
  TVectorD temp_z = qr.Solve(b, ok);
  if (!ok) {
    TDecompSVD svd(AP);
    temp_z = svd.Solve(b, ok);
    if (!ok) throw std::runtime_error("NNLS: least-squares solve failed (QR & SVD).");
  }

  // Manual copy for numerical stability
  z.ResizeTo(temp_z.GetNrows());
  for (int i = 0; i < temp_z.GetNrows(); ++i) {
    z[i] = temp_z[i];
  }
  return z;
}

// Compute infinity norm of vector
static double InfNorm(const TVectorD& v) {
  double norm = 0.0;
  for (int i = 0; i < v.GetNrows(); ++i) {
    norm = std::max(norm, std::abs(v[i]));
  }
  return norm;
}

}  // namespace

TVectorD NNLS_LawsonHanson(const TMatrixD& A, const TVectorD& b, double tol, int max_outer) {
  const int m = A.GetNrows();
  const int n = A.GetNcols();
  if (b.GetNrows() != m) {
    throw std::invalid_argument("NNLS: Matrix A and vector b dimensions are incompatible.");
  }

  // Initialize state vectors
  TVectorD x(n);
  x.Zero();
  TVectorD r(m);
  for (int i = 0; i < m; ++i) r[i] = b[i];  // r = b - A*x (x starts at zero)

  // Compute gradient w = A^T * r
  TVectorD w(n);
  for (int j = 0; j < n; ++j) {
    w[j] = 0.0;
    for (int i = 0; i < m; ++i) {
      w[j] += A(i, j) * r[i];
    }
  }

  // Active set management: P = passive set (in solution), Z = zero set (complement)
  std::vector<char> inP(n, 0);  // Boolean flags for passive set membership
  std::vector<int> Pidx;        // Indices of variables in passive set

  // Algorithm parameters and tolerances
  if (tol < 0.0) {
    // Adaptive tolerance based on gradient magnitude
    const double atb_norm = InfNorm(w);
    tol = std::max(1e-15, 1e-12 * atb_norm);
  }
  if (max_outer <= 0) max_outer = 3 * std::max(1, n);

  // Early termination if optimality conditions already satisfied
  if (InfNorm(w) <= tol) {
    return x;
  }

  // Pre-allocate workspace matrices for efficiency
  TMatrixD AP;
  int outer_iter = 0;

  // Main optimization loop
  while (true) {
    // Find variable with most positive gradient (steepest ascent direction)
    int t = -1;
    double wmax = tol;  // Only consider gradients above tolerance
    for (int j = 0; j < n; ++j) {
      if (!inP[j] && w[j] > wmax) {
        wmax = w[j];
        t = j;
      }
    }

    // Check termination conditions
    if (t < 0 || outer_iter++ >= max_outer) break;

    // Add variable t to passive set
    inP[t] = 1;
    Pidx.clear();
    Pidx.reserve(n);
    for (int j = 0; j < n; ++j) {
      if (inP[j]) Pidx.push_back(j);
    }

    // Inner loop: solve unconstrained problem on passive set with feasibility enforcement
    while (true) {
      // Solve least squares subproblem: minimize ||A_P * z - b||^2
      ColSubset(A, Pidx, AP);
      TVectorD zP = SolveLeastSquares(AP, b);

      // Reconstruct full solution vector
      TVectorD z(n);
      z.Zero();
      for (int k = 0; k < (int)Pidx.size(); ++k) {
        z[Pidx[k]] = zP[k];
      }

      // Check feasibility: if all passive variables are non-negative, accept solution
      bool allpos = true;
      for (int k = 0; k < (int)Pidx.size(); ++k) {
        if (zP[k] <= 0.0) {
          allpos = false;
          break;
        }
      }
      if (allpos) {
        x = z;
        break;
      }

      // Feasibility enforcement: find maximum step size that maintains non-negativity
      double alpha = 1.0;
      for (int j = 0; j < n; ++j) {
        if (inP[j] && z[j] <= 0.0) {
          const double denom = x[j] - z[j];
          if (denom > 0.0) alpha = std::min(alpha, x[j] / denom);
        }
      }

      // Take step toward unconstrained solution
      for (int j = 0; j < n; ++j) {
        x[j] = x[j] + alpha * (z[j] - x[j]);
      }

      // Remove variables that have become zero from passive set
      const double numerical_zero = 1e-15;
      bool removed = false;
      for (int j = 0; j < n; ++j) {
        if (inP[j] && x[j] <= numerical_zero) {
          inP[j] = 0;
          x[j] = 0.0;  // Ensure exact zero
          removed = true;
        }
      }

      if (removed) {
        // Rebuild passive set index list
        Pidx.clear();
        for (int j = 0; j < n; ++j) {
          if (inP[j]) Pidx.push_back(j);
        }
        if (Pidx.empty()) break;  // No variables left in passive set
      }
    }

    // Update residual and gradient for next iteration
    if (outer_iter < max_outer) {
      // Compute residual r = b - A*x
      r.ResizeTo(b.GetNrows());
      for (int i = 0; i < b.GetNrows(); ++i) {
        r[i] = b[i];
        for (int j = 0; j < A.GetNcols(); ++j) {
          r[i] -= A(i, j) * x[j];
        }
      }

      // Compute gradient w = A^T * r
      w.ResizeTo(A.GetNcols());
      for (int j = 0; j < A.GetNcols(); ++j) {
        w[j] = 0.0;
        for (int i = 0; i < A.GetNrows(); ++i) {
          w[j] += A(i, j) * r[i];
        }
      }
    }
  }

  // Ensure final solution satisfies non-negativity constraints
  for (int j = 0; j < n; ++j) {
    if (x[j] < 0.0) x[j] = 0.0;
  }

  return x;
}

double NNLS_ResidualNorm(const TMatrixD& A, const TVectorD& b, const TVectorD& x) {
  // Compute residual r = b - A*x
  TVectorD r(b.GetNrows());
  for (int i = 0; i < b.GetNrows(); ++i) {
    r[i] = b[i];
    for (int j = 0; j < A.GetNcols(); ++j) {
      r[i] -= A(i, j) * x[j];
    }
  }

  // Compute Euclidean norm
  double norm_sq = 0.0;
  for (int i = 0; i < r.GetNrows(); ++i) {
    norm_sq += r[i] * r[i];
  }
  return std::sqrt(norm_sq);
}

}  // namespace Math
}  // namespace RAT
