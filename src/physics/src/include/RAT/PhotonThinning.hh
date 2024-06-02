#ifndef __RAT_PhotonThinning__
#define __RAT_PhotonThinning__

namespace RAT {

class PhotonThinning {
 public:
  static void Init();

  inline static double GetFactor() { return fThinFactor; };
  static void SetFactor(double factor);

  inline static double GetCherenkovThinningFactor() { return fCherenkovThinningFactor; };
  static void SetCherenkovThinningFactor(double factor);
  inline static double GetScintillationThinningFactor() { return fScintillationThinningFactor; };
  static void SetScintillationThinningFactor(double factor);

  inline static double GetCherenkovLowerWavelengthThreshold() { return fCherenkovLowerWavelengthThreshold; };
  static void SetCherenkovLowerWavelengthThreshold(double thresh);
  inline static double GetScintillationLowerWavelengthThreshold() { return fScintillationLowerWavelengthThreshold; };
  static void SetScintillationLowerWavelengthThreshold(double thresh);

  inline static double GetCherenkovUpperWavelengthThreshold() { return fCherenkovUpperWavelengthThreshold; };
  static void SetCherenkovUpperWavelengthThreshold(double thresh);
  inline static double GetScintillationUpperWavelengthThreshold() { return fScintillationUpperWavelengthThreshold; };
  static void SetScintillationUpperWavelengthThreshold(double thresh);

 protected:
  static double fThinFactor;

  static double fCherenkovThinningFactor;
  static double fScintillationThinningFactor;

  static double fCherenkovLowerWavelengthThreshold;
  static double fScintillationLowerWavelengthThreshold;

  static double fCherenkovUpperWavelengthThreshold;
  static double fScintillationUpperWavelengthThreshold;
};

}  // namespace RAT

#endif
