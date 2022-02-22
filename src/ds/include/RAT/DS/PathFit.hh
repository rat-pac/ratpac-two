/**
 *  @class DS::PathFit
 *
 *  Data Structure: Position fit using FTP
 */
#ifndef __RAT_DS_PathFit__
#define __RAT_DS_PathFit__

#include <RAT/DS/PosFit.hh>

namespace RAT {
namespace DS {

class PathFit : public TObject, public PosFit {
  public:
    PathFit() : TObject(), PosFit("fitpath") {}
    virtual ~PathFit() {}

    /* Position attributes inherited from PosFit */

    virtual double GetTime0() const { return time0; }
    virtual void SetTime0(double _time0) { time0 = _time0; }
    
    virtual TVector3 GetPos0() const { return pos0; }
    virtual void SetPos0(TVector3 _pos0) { pos0 = _pos0; }

    virtual double GetTime() const { return time; }
    virtual void SetTime(double _time) { time = _time; }

    virtual TVector3 GetDirection() const { return dir; }
    virtual void SetDirection(TVector3 _dir) { dir = _dir; }

    virtual double GetGoodness() const { return goodness; }
    virtual void SetGoodness(double _goodness) { goodness = _goodness; }

    virtual double GetdLFromVtx() const { return dlfromvtx; }
    virtual void SetdLFromVtx(double _dlfromvtx) { dlfromvtx = _dlfromvtx; }

    ClassDef(PathFit, 1)
  protected:
    double time0;
    double time;
    double goodness;
    double dlfromvtx;
    TVector3 pos0;
    TVector3 dir;
};

} // namespace DS
} // namespace RAT

#endif
