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

 protected:
  static double fThinFactor;
  static double fCherenkovThinningFactor;
  static double fScintillationThinningFactor;
};

}  // namespace RAT

#endif
