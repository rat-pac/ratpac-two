/**
 * @class DS::NestedTubeInfo
 * Data Structure: Fiber properties
 *
 * Information about nested tubes (fibers), including positions, rotations, and lengths
 */

#ifndef __RAT_DS_NestedTubeInfo__
#define __RAT_DS_NestedTubeInfo__

#include <TObject.h>

#include <G4ThreeVector.hh>
#include <algorithm>

namespace RAT {
namespace DS {

class NestedTubeInfo : public TObject {
 public:
  NestedTubeInfo() : TObject() {}
  virtual ~NestedTubeInfo() {}

  virtual void AddNestedTube(const G4ThreeVector& _pos, const G4ThreeVector& _dir, const double _length,
                             const double _core_r, const double _inner_r, const double _outer_r,
                             const std::string _core_material, const std::string _inner_material,
                             const std::string _outer_material) {
    pos.push_back(_pos);
    dir.push_back(_dir);
    length.push_back(_length);
    core_r.push_back(_core_r);
    inner_r.push_back(_inner_r);
    outer_r.push_back(_outer_r);
    core_material.push_back(_core_material);
    inner_material.push_back(_inner_material);
    outer_material.push_back(_outer_material);
  }

  virtual void AddNestedTube(const G4ThreeVector& _pos, const G4ThreeVector& _dir, const int _length) {
    AddNestedTube(_pos, _dir, _length, 0.47, 0.485, 0.5, "", "", "");
  }

  virtual Int_t GetNestedTubeCount() const { return pos.size(); }

  virtual G4ThreeVector GetPosition(int id) const { return pos.at(id); }
  virtual void SetPosition(int id, const G4ThreeVector& _pos) { pos.at(id) = _pos; }

  virtual G4ThreeVector GetDirection(int id) const { return dir.at(id); }
  virtual void SetDirection(int id, const G4ThreeVector& _dir) { dir.at(id) = _dir; }

  virtual int GetLength(int id) const { return length.at(id); }
  virtual void SetLength(int id, int _length) { length.at(id) = _length; }

  virtual double GetCoreR(int id) const { return core_r.at(id); }
  virtual void SetCoreR(int id, double _core_r) { core_r.at(id) = _core_r; }

  virtual double GetInnerR(int id) const { return inner_r.at(id); }
  virtual void SetInnerR(int id, double _inner_r) { inner_r.at(id) = _inner_r; }

  virtual double GetOuterR(int id) const { return outer_r.at(id); }
  virtual void SetOuterR(int id, double _outer_r) { outer_r.at(id) = _outer_r; }

  virtual std::string GetCoreMaterial(int id) const { return core_material.at(id); }
  virtual void SetCoreMaterial(int id, double _core_material) { core_material.at(id) = _core_material; }

  virtual std::string GetInnerMaterial(int id) const { return inner_material.at(id); }
  virtual void SetInnerMaterial(int id, double _inner_material) { inner_material.at(id) = _inner_material; }

  virtual std::string GetOuterMaterial(int id) const { return outer_material.at(id); }
  virtual void SetOuterMaterial(int id, double _outer_material) { outer_material.at(id) = _outer_material; }

  ClassDef(NestedTubeInfo, 2);

 protected:
  std::vector<G4ThreeVector> pos;
  std::vector<G4ThreeVector> dir;
  std::vector<double> length;
  std::vector<double> core_r;
  std::vector<double> inner_r;
  std::vector<double> outer_r;
  std::vector<std::string> core_material;
  std::vector<std::string> inner_material;
  std::vector<std::string> outer_material;
};

}  // namespace DS
}  // namespace RAT

#endif
