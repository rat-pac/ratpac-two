/**
 *
 * @brief A class for performing 1D FFTs using the FFTW library.
 * @author James Shen <jierans@sas.upenn.edu>
 *
 * FFTW has the concept of a `plan`, which is a pre-computed set of instructions coupled to specific input and output
 * memroy buffers. Compared to the execution of a plan, the creation of the plan is much more expensive. This class is a
 * RAII style wrapper around a FFTW plan. It lives and dies with a fftw plan, and manages the input/output buffers
 * associated with it. It provides transformation fucntions that correctly pads the input data to the size expected by
 * the plan.
 *
 * */

#ifndef __RAT_FFTW1DTRANSFORMER_HH__
#define __RAT_FFTW1DTRANSFORMER_HH__

#include <fftw3.h>

#include <RAT/Log.hh>
#include <complex>
#include <memory>
#include <stdexcept>
#include <vector>

class FFTW1DTransformer {
 public:
  /**
   * @brief Represents the direction of the FFT transformation.
   * */
  enum class direction_t {
    FORWARD = FFTW_FORWARD,  /// @brief Forward FFT Transformation. Real to complex.
    INVERSE = FFTW_BACKWARD  /// @brief Inverse FFT Transformation. Complex to real.
  };

  /**
   * @brief Constructor for FFTW1DTransformer.
   * @param size: Size of the FFT (must be a power of two). Note: this is always defined in the time domain. The freq
   *              domain will have length size/2 + 1.
   * @param direction: Direction of the FFT (default is FORWARD).
   **/
  FFTW1DTransformer(size_t size, direction_t direction = direction_t::FORWARD, unsigned flag = FFTW_MEASURE)
      : fft_size(size), fft_direction(direction) {
    if (size == 0) {
      throw std::invalid_argument("FFTW1DTransformer: size must be greater than 0");
    }
    if ((size & (size - 1)) != 0) {  // bitwise check for power of two
      RAT::info << "FFTW1DTransformer: size is not a power of two: " << size
                << ". This may lead to suboptimal performance." << newline;
    }
    rbuf = std::unique_ptr<double[], buf_deleter>(fftw_alloc_real(size));
    cbuf = std::unique_ptr<fftw_complex[], buf_deleter>(fftw_alloc_complex(size / 2 + 1));
    if (direction == direction_t::FORWARD) {
      plan = fftw_plan_dft_r2c_1d(static_cast<int>(size), rbuf.get(), cbuf.get(), flag);
    } else {
      plan = fftw_plan_dft_c2r_1d(static_cast<int>(size), cbuf.get(), rbuf.get(), flag);
    }

    if (!plan) {
      throw std::runtime_error("FFTW1DTransformer: Failed to create FFTW plan");
    }
  }

  ~FFTW1DTransformer() { fftw_destroy_plan(plan); }

  size_t GetSize() const { return fft_size; }

  /// @brief Forward transformation.
  std::vector<std::complex<double>> transform(const std::vector<double>& input) {
    if (fft_direction != direction_t::FORWARD) {
      throw std::runtime_error("FFTW1DTransformer: This transformer is not configured for forward FFT.");
    }

    if (input.size() > fft_size) {
      throw std::invalid_argument("FFTW1DTransformer: Input size exceeds FFT size.");
    }

    std::fill(rbuf.get(), rbuf.get() + fft_size, 0.0);
    std::copy(input.begin(), input.end(), rbuf.get());

    fftw_execute(plan);

    std::vector<std::complex<double>> result(fft_size / 2 + 1);
    // assumes fftw_complex is directly compatible with std::complex<double>
    std::copy(reinterpret_cast<std::complex<double>*>(cbuf.get()),
              reinterpret_cast<std::complex<double>*>(cbuf.get() + (fft_size / 2 + 1)), result.begin());
    return result;
  }

  /// @brief Inverse transformation.
  std::vector<double> transform(const std::vector<std::complex<double>>& input) {
    if (fft_direction != direction_t::INVERSE) {
      throw std::runtime_error("FFTW1DTransformer: This transformer is not configured for inverse FFT.");
    }

    if (input.size() != fft_size / 2 + 1) {
      throw std::invalid_argument("FFTW1DTransformer: Input size does not match FFT size for inverse transform.");
    }

    std::copy(input.begin(), input.end(), reinterpret_cast<std::complex<double>*>(cbuf.get()));
    fftw_execute(plan);
    std::vector<double> result(fft_size);
    std::copy(rbuf.get(), rbuf.get() + fft_size, result.begin());

    // normalize the result
    for (double& val : result) val /= static_cast<double>(fft_size);
    return result;
  }

 protected:
  // Deleter for managing i/o buffers.
  struct buf_deleter {
    void operator()(double* p) const { fftw_free(p); }
    void operator()(fftw_complex* p) const { fftw_free(p); }
  };
  size_t fft_size;
  direction_t fft_direction;
  fftw_plan plan;
  std::unique_ptr<double[], buf_deleter> rbuf;        // for time domain.
  std::unique_ptr<fftw_complex[], buf_deleter> cbuf;  // for frequency domain.
};

#endif  // __RAT_FFTW1DTRANSFORMER_HH__
