/**
 * @class DS::MCPhoton
 * Data Structure: Photoelectron generated at PMT.
 *
 * @author Stan Seibert <sseibert@hep.upenn.edu>
 *
 * This class represents a single photoelectron generated at the
 * photocathode of the PMT.  The time jitter and delay in transit to the
 * anode are not included here, but the distribution of charge is,
 * which is slightly incongruous.

 * Note that we require that the photon generates a photoelectron.
 * Absorbed photons are not included here.
 */

#ifndef __RAT_DS_MCPhoton__
#define __RAT_DS_MCPhoton__

#include <TObject.h>
#include <TVector3.h>

namespace RAT {
namespace DS {

class MCPhoton : public TObject {
 public:
  MCPhoton() : TObject(), isDarkHit(false) {}
  virtual ~MCPhoton() {}

  /** Time of photon hit at photocathode relative to event start time (ns). */
  virtual Double_t GetHitTime() const { return hitTime; }
  virtual void SetHitTime(Double_t _hitTime) { hitTime = _hitTime; }

  /** Time of pulse arriving at front-end electronics. */
  virtual Double_t GetFrontEndTime() const { return frontEndTime; }
  virtual void SetFrontEndTime(Double_t _frontEndTime) { frontEndTime = _frontEndTime; }

  /** Location of photon hit in local PMT coordinates (mm). */
  virtual TVector3 GetPosition() const { return pos; }
  virtual void SetPosition(const TVector3 &_pos) { pos = _pos; }

  /** Wavelength of photon (mm). */
  virtual Double_t GetLambda() const { return lambda; }
  virtual void SetLambda(Double_t _lambda) { lambda = _lambda; }

  /** Momentum of photon (MeV/c). */
  virtual TVector3 GetMomentum() const { return mom; }
  virtual void SetMomentum(const TVector3 &_mom) { mom = _mom; }

  /** Polarization vector. */
  virtual TVector3 GetPolarization() const { return pol; }
  virtual void SetPolarization(const TVector3 &_pol) { pol = _pol; }

  /** Charge created by photon in photoelectron (pe) units.
   *
   *  One pe is defined to be the peak of the single photoelectron
   *  charge distribution for this PMT.
   */
  virtual Double_t GetCharge() const { return charge; }
  virtual void SetCharge(Double_t _charge) { charge = _charge; }

  /** Is this photoelectron due to a dark hit? */
  virtual void SetDarkHit(Bool_t _isDarkHit) { isDarkHit = _isDarkHit; }
  virtual Bool_t IsDarkHit() const { return isDarkHit; }

  /** Is this photoelectron due to an after-pulse? */
  virtual void SetAfterPulse(Bool_t _isAfterPulse) { isAfterPulse = _isAfterPulse; }
  virtual Bool_t IsAfterPulse() const { return isAfterPulse; }

  /** Track ID of photon which generated this photoelectron */
  virtual void SetTrackID(Int_t _trackID) { trackID = _trackID; }
  virtual Int_t GetTrackID() const { return trackID; }

  /** Name of physics process acting at endpoint of the MCTrackStep
   * that created this photon hit.
   */
  virtual std::string GetCreatorProcess() const { return process; }
  virtual void SetCreatorProcess(const std::string &_process) { process = _process; }

  /** Creation time of this PE */
  virtual void SetCreationTime(Double_t _creationTime) { creationTime = _creationTime; }
  virtual Double_t GetCreationTime() const { return creationTime; }

  /** Operator overload **/
  bool operator<(const MCPhoton &mcp) const { return (frontEndTime < mcp.frontEndTime); }
  bool operator>(const MCPhoton &mcp) const { return (frontEndTime > mcp.frontEndTime); }

  ClassDef(MCPhoton, 4);

 protected:
  Double_t hitTime;
  Double_t frontEndTime;
  Double_t creationTime;
  Double_t lambda;
  TVector3 pos;
  TVector3 mom;
  TVector3 pol;

  Double_t charge;
  Bool_t isDarkHit;
  Bool_t isAfterPulse;
  Int_t trackID;
  std::string process;
};

}  // namespace DS
}  // namespace RAT

#endif
