/**
 * @class DS::PMTInfo
 * Data Structure: PMT properties
 *
 * Information about PMTs, including positions, rotations, and types
 */

#ifndef __RAT_DS_PMTInfo__
#define __RAT_DS_PMTInfo__

#include <TObject.h>
#include <TVector3.h>

#include <algorithm>

namespace RAT {
namespace DS {

class PMTInfo : public TObject {
 public:
  PMTInfo() : TObject() {}
  virtual ~PMTInfo() {}

  virtual void AddPMT(const TVector3& _pos, const TVector3& _dir, const int _type, const int _ch,
                      const std::string _model, const double _individual_efficiency_corr,
                      const double _individual_noise_rate, const double _individual_afterpulse_fraction) {
    pos.push_back(_pos);
    dir.push_back(_dir);
    type.push_back(_type);
    individual_efficiency_corr.push_back(_individual_efficiency_corr);
    individual_noise_rate.push_back(_individual_noise_rate);
    individual_afterpulse_fraction.push_back(_individual_afterpulse_fraction);
    channel_num.push_back(_ch);
    std::vector<std::string>::iterator which = std::find(models.begin(), models.end(), _model);
    if (which != models.end()) {
      modeltype.push_back(which - models.begin());
    } else {
      models.push_back(_model);
      modeltype.push_back(models.size() - 1);
    }
  }

  virtual void AddPMT(const TVector3& _pos, const TVector3& _dir, const int _type) {
    AddPMT(_pos, _dir, _type, -1, "", 1.0, 0.0, 0.0);
  }

  virtual Int_t GetPMTCount() const { return pos.size(); }

  virtual TVector3 GetPosition(int id) const { return pos.at(id); }
  virtual void SetPosition(int id, const TVector3& _pos) { pos.at(id) = _pos; }

  virtual TVector3 GetDirection(int id) const { return dir.at(id); }
  virtual void SetDirection(int id, const TVector3& _dir) { dir.at(id) = _dir; }

  virtual int GetChannelNumber(int id) const { return channel_num.at(id); }
  virtual void SetChannelNumber(int id, int _ch) { channel_num.at(id) = _ch; }

  virtual int GetType(int id) const { return type.at(id); }
  virtual void SetType(int id, int _type) { type.at(id) = _type; }

  virtual double GetEfficiencyCorr(int id) const { return individual_efficiency_corr.at(id); }
  virtual void SetEfficiencyCorr(int id, double _individual_efficiency_corr) {
    individual_efficiency_corr.at(id) = _individual_efficiency_corr;
  }

  virtual double GetNoiseRate(int id) const { return individual_noise_rate.at(id); }
  virtual void SetNoiseRate(int id, double _rate) { individual_noise_rate.at(id) = _rate; }

  virtual double GetAfterPulseFraction(int id) const { return individual_afterpulse_fraction.at(id); }
  virtual void SetAfterPulseFraction(int id, double _frac) { individual_afterpulse_fraction.at(id) = _frac; }

  virtual int GetModel(int id) const { return modeltype.at(id); }
  virtual int SetModel(int id, std::string _model) {
    std::vector<std::string>::iterator which = std::find(models.begin(), models.end(), _model);
    int _modeltype;
    if (which != models.end()) {
      _modeltype = which - models.begin();
    } else {
      models.push_back(_model);
      _modeltype = models.size() - 1;
    }
    modeltype.at(id) = _modeltype;
    return _modeltype;
  }
  virtual std::string GetModelName(int _modeltype) const { return models.at(_modeltype); }
  virtual int GetModelCount() const { return models.size(); }

  virtual std::string GetModelNameByID(int id) const { return GetModelName(GetModel(id)); }

  ClassDef(PMTInfo, 3);

 protected:
  std::vector<TVector3> pos;
  std::vector<TVector3> dir;
  std::vector<int> type;
  std::vector<int> channel_num;
  std::vector<int> modeltype;
  std::vector<std::string> models;
  std::vector<double> individual_efficiency_corr;
  std::vector<double> individual_noise_rate;
  std::vector<double> individual_afterpulse_fraction;
};

}  // namespace DS
}  // namespace RAT

#endif
