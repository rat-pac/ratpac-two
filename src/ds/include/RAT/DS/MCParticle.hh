/**
 * @class DS::MCParticle
 * Data Structure: Initial particle state
 *
 * @author Stan Seibert <sseibert@hep.upenn.edu>
 *
 * This class describes the state of one of the initial particles in this event.
 * Secondaries created during simulation are not included here.
 */

#ifndef __RAT_DS_MCParticle__
#define __RAT_DS_MCParticle__

#include <TObject.h>
#include <TVector3.h>

#include <string>

namespace RAT {
namespace DS {

class MCParticle : public TObject {
 public:
  MCParticle() : TObject() {}
  virtual ~MCParticle() {}

  /**
   * Particle type
   *
   * Use integer specified by PDG at
   * http://pdg.lbl.gov/2005/reviews/montecarlorpp.pdf
   */
  virtual Int_t GetPDGCode() const { return pdgcode; }
  virtual void SetPDGCode(Int_t _pdgcode) { pdgcode = _pdgcode; }

  virtual const std::string &GetParticleName() const { return particleName; }
  virtual void SetParticleName(const std::string &_particleName) { particleName = _particleName; }

  /** Initial time of particle (ns). */
  virtual Double_t GetTime() const { return t; }
  virtual void SetTime(Double_t _t) { t = _t; }

  /** Initial location of particle (mm). */
  virtual TVector3 GetPosition() const { return pos; }
  virtual void SetPosition(const TVector3 &_pos) { pos = _pos; }

  /** Initial kinetic energy of particle (MeV). */
  virtual Double_t GetKE() const { return ke; }
  virtual void SetKE(Double_t _ke) { ke = _ke; }

  /** Initial momentum of particle (MeV/c) */
  virtual TVector3 GetMomentum() const { return mom; }
  virtual void SetMomentum(const TVector3 &_mom) { mom = _mom; }

  /** End time of particle (ns). */
  virtual Double_t GetEndTime() const { return end_t; }
  virtual void SetEndTime(Double_t _t) { end_t = _t; }

  /** End location of particle (mm). */
  virtual TVector3 GetEndPosition() const { return end_pos; }
  virtual void SetEndPosition(const TVector3 &_pos) { end_pos = _pos; }

  /** End kinetic energy of particle (MeV). */
  virtual Double_t GetEndKE() const { return end_ke; }
  virtual void SetEndKE(Double_t _ke) { end_ke = _ke; }

  /** End momentum of particle (MeV/c) */
  virtual TVector3 GetEndMomentum() const { return end_mom; }
  virtual void SetEndMomentum(const TVector3 &_mom) { end_mom = _mom; }

  /** Polarization vector */
  virtual TVector3 GetPolarization() const { return pol; }
  virtual void SetPolarization(const TVector3 &_pol) { pol = _pol; }

  ClassDef(MCParticle, 3);

 protected:
  Int_t pdgcode;
  Double_t t;
  Double_t ke;
  TVector3 pos;
  TVector3 mom;
  Double_t end_t;
  Double_t end_ke;
  TVector3 end_pos;
  TVector3 end_mom;
  TVector3 pol;
  std::string particleName;
};

}  // namespace DS
}  // namespace RAT

#endif
