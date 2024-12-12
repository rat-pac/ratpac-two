/**@file
   This file defines global position generator for primary events.
   (See note on GenericLAND generators for more information.)

   This file is part of the GenericLAND software library.
   $Id: GLG4PosGen.cc,v 1.2 2005/11/19 17:59:39 volsung Exp $

   @author G.Horton-Smith, August 3, 2001
*/

#include "RAT/GLG4PosGen.hh"

#include <CLHEP/Units/SystemOfUnits.h>

#include <sstream>

#include "G4AffineTransform.hh"
#include "G4GeometryTolerance.hh"
#include "G4Material.hh"
#include "G4Navigator.hh"
#include "G4PrimaryVertex.hh"
#include "G4TransportationManager.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VoxelLimits.hh"
#include "RAT/GLG4StringUtil.hh"
#include "RAT/GLG4VertexGen.hh"  // for GLG4VertexGen_HEPEvt
#include "RAT/Log.hh"
#include "Randomize.hh"

// To support GEANT4.6 and up
#define std std

GLG4PosGen_Point::GLG4PosGen_Point(const char *arg_dbname) : GLG4PosGen(arg_dbname), _fixedPos(0., 0., 0.) {}

void GLG4PosGen_Point::GeneratePosition(G4ThreeVector &argResult) { argResult = _fixedPos; }

void GLG4PosGen_Point::SetState(G4String newValues) {
  newValues = util_strip_default(newValues);
  if (newValues.length() == 0) {
    // print help and current state
    RAT::info << "Current state of this GLG4PosGen_Point:\n"
              << " \"" << GetState() << "\"\n"
              << newline;
    RAT::info << "Format of argument to GLG4PosGen_Point::SetState: \n"
                 " \"x_mm y_mm z_mm\""
              << newline;
    return;
  }

  std::istringstream is(newValues.c_str());

  // set position
  G4double x, y, z;
  is >> x >> y >> z;
  if (is.fail()) {
    RAT::warn << "GLG4PosGen_Point::SetState: "
                 "Could not parse three floats from input std::string"
              << newline;
    return;
  }
  _fixedPos = G4ThreeVector(x, y, z);
}

G4String GLG4PosGen_Point::GetState() const {
  return util_dformat("%ld\t%ld\t%ld", _fixedPos.x(), _fixedPos.y(), _fixedPos.z());
}

////////////////////////////////////////////////////////////////

GLG4PosGen_Paint::GLG4PosGen_Paint(const char *arg_dbname)
    : GLG4PosGen(arg_dbname),
      _pos(0., 0., 0.),
      _thickness(0),
      _pVolumeName("!"),
      _pVolume(0),
      _materialName(""),
      _material(0),
      _ntried(0),
      _nfound(0),
      _boundingBoxVolume(0.0) {}

void GLG4PosGen_Paint::GeneratePosition(G4ThreeVector &argResult) {
  double surfaceTolerance = G4GeometryTolerance::GetInstance()->GetSurfaceTolerance();

  // We need to refer to physical volume.  First set up the navigator
  // to know the surrounding volume and heirarchy (Note this must
  // always be done, even if we already know _pVolume.)
  G4VPhysicalVolume *pv;  // local variable for speed and later use
  G4Navigator *gNavigator = G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking();
  pv = gNavigator->LocateGlobalPointAndSetup(_pos, 0, false);  // full setup
  if (pv == 0) {
    G4cerr << "GLG4PosGen_Paint: "
              "Could not find any volume at "
           << _pos << newline;
    return;
  }

  // check or set related field variables
  if (pv != _pVolume) {   // normally happens just once after a SetState()
    if (_pVolume == 0) {  // SetState() sets _pVolume==0
      _pVolume = pv;
      if (_pVolumeName.length() == 0 || _pVolumeName == "!") _pVolumeName = _pVolume->GetName();
      if (_pVolumeName != _pVolume->GetName()) {
        G4cerr << "Warning: actual volume at " << _pos << " is " << _pVolume->GetName()
               << ", not equal to expected volume " << _pVolumeName << " in GLG4PosGen_Paint." << newline;
      }
    } else {
      G4cerr << "Warning: actual volume at " << _pos << " has changed! Now " << pv->GetName() << ", was "
             << _pVolume->GetName() << " in GLG4PosGen_Paint." << newline;
      _pVolume = pv;
      _pVolumeName = pv->GetName();
    }
    if (_materialName.length() > 0 && _material == 0) {
      _material = G4Material::GetMaterial(_materialName);
      if (_material == 0) {
        RAT::warn << "ERROR from GLG4PosGen_Paint: no material named " << _materialName
                  << ", material restriction cancelled." << newline;
        _materialName = "";
      }
    }
    _boundingBoxVolume = -1.0;
  }

  // here are some more things that both Paint and Fill algorithms need
  G4VSolid *solid = pv->GetLogicalVolume()->GetSolid();                         // this is the solid
  G4AffineTransform local_to_global = gNavigator->GetLocalToGlobalTransform();  // note Navigator init'ed above
  G4double x0, dx, y0, dy, z0, dz;                                              // for bounding box of solid
  {
    // get bounding box
    G4VoxelLimits voxelLimits;          // Defaults to "infinite" limits.
    G4AffineTransform affineTransform;  // Defaults to no transform
    G4double tmax;
    solid->CalculateExtent(kXAxis, voxelLimits, affineTransform, x0, tmax);
    dx = tmax - x0;
    solid->CalculateExtent(kYAxis, voxelLimits, affineTransform, y0, tmax);
    dy = tmax - y0;
    solid->CalculateExtent(kZAxis, voxelLimits, affineTransform, z0, tmax);
    dz = tmax - z0;
    if (_boundingBoxVolume <= 0.0)
      _boundingBoxVolume = dx * dy * dz;
    else if (fabs(dx * dy * dz / _boundingBoxVolume - 1.0) > 1e-6) {  // paranoia check
      RAT::warn << "Warning: volume of bounding box changed for " << _pVolumeName << " -- was " << _boundingBoxVolume
                << ", now " << dx * dy * dz << ", fractional change " << fabs(dx * dy * dz / _boundingBoxVolume - 1.0)
                << " -- PARANOIA IS JUSTIFIED!!!!" << newline;
      _boundingBoxVolume = dx * dy * dz;
    }
  }

  // a complicated case: generate points uniformly on the
  // surface of the surrounding volume.  The mathematical technique
  // used here relies on what I think of as "Olber's Law":
  // given a surrounding surface with uniform # sources per unit area and
  // cos(theta) distribution of emitted rays (theta is angle from normal),
  // the brightness per unit area on the surface of a contained volume
  // is uniform (and the incident rays have a cos(theta) distribution,
  // although this is largely irrelevant for us).  This is true even for
  // non-convex solids if one considers all intercepts, such that the
  // interior solid does not shadow itself.

  G4ThreeVector rpos;  // this will hold the position result
  for (int iloop = 0, jloop = 0; /* test inside */; iloop++) {
    // First decide whether we will use a stacked intercept or trace a new
    // ray.  The following procedure avoids using consecutive intercepts
    // from the same ray; in order to avoid an ever-increasing list size,
    // it is more likely to use an existing intercept as the number of
    // intercepts grows.
    unsigned iint = (unsigned)((_intercepts.size() + 2) * G4UniformRand());
    if (iint >= _intercepts.size()) {
      double Rsphere = 0.50001 * sqrt(dx * dx + dy * dy + dz * dz) + surfaceTolerance;
      G4ThreeVector sphere_center(x0 + 0.5 * dx, y0 + 0.5 * dy, z0 + 0.5 * dz);
      while (iint >= _intercepts.size()) {
        // "infinite" loop test
        jloop++;
        if (jloop >= 100000) {
          RAT::warn << "GLG4PosGen_PointPaintFill::GeneratePosition(): " << iloop << "," << jloop
                    << " loops spent looking for point within " << _thickness << "-mm-thick surface layer of "
                    << _pVolumeName;
          if (_material) RAT::warn << " with material " << _materialName;
          RAT::warn << newline;

          argResult = rpos;
          return;
        }

        // make a new ray and add any intercepts; repeat until we have enough.
        // generate direction:
        // the following cute sequence generates a cos(theta) distribution!
        double u, v, w;
        do {
          u = G4UniformRand() * 2.0 - 1.0;
          v = G4UniformRand() * 2.0 - 1.0;
          w = 1.0 - (u * u + v * v);
        } while (w < 0.0);
        w = sqrt(w);
        // (end of cute sequence.)
        // generate position on unit sphere:
        G4ThreeVector spos;
        double r2;
        do {
          spos = G4ThreeVector(G4UniformRand() * 2.0 - 1.0, G4UniformRand() * 2.0 - 1.0, G4UniformRand() * 2.0 - 1.0);
          r2 = spos.mag2();
        } while (r2 > 1.0 || r2 < 0.0625);
        spos *= sqrt(1.0 / r2);
        // rotate direction to be relative to normal
        G4ThreeVector e1 = spos.orthogonal().unit();
        G4ThreeVector e2 = spos.cross(e1);
        G4ThreeVector raydir = u * e1 + v * e2 - w * spos;
        // scale and offset position to be on surrounding sphere
        spos *= Rsphere;
        spos += sphere_center;
        // now find intercepts
        for (int isafety = 0; isafety < 20; isafety++) {
          // usually will exit loop as soon as no intercept found,
          // unless there is a bug in a geometry routine
          double dist = solid->DistanceToIn(spos, raydir);
          if (dist >= 2.0 * Rsphere) break;
          if (dist <= 0.0) {
            G4cerr << "GLG4PosGen_PointPaintFill: strange DistanceToIn " << dist << " on " << _pVolumeName << " at "
                   << spos << " in dir " << raydir << " loop " << isafety << newline;
          }
          if (dist > surfaceTolerance) {
            spos += dist * raydir;
            if (_thickness == 0.0)
              _intercepts.push_back(spos);
            else
              _intercepts.push_back(spos + solid->SurfaceNormal(spos) * G4UniformRand() * _thickness);
          } else {
            spos += surfaceTolerance * raydir;
          }

          dist = solid->DistanceToOut(spos, raydir);
          if (dist >= 2.0 * Rsphere) break;
          if (dist <= 0.0) {
            G4cerr << "GLG4PosGen_PointPaintFill: strange DistanceToOut " << dist << " on " << _pVolumeName << " at "
                   << spos << " in dir " << raydir << " loop " << isafety << newline;
          }
          if (dist > surfaceTolerance) {
            spos += dist * raydir;
            if (_thickness == 0.0)
              _intercepts.push_back(spos);
            else
              _intercepts.push_back(spos + solid->SurfaceNormal(spos) * G4UniformRand() * _thickness);
          } else {
            spos += surfaceTolerance * raydir;
          }
        }
        // have now added any and all intercepts for this ray
      }
      // now have enough intercepts to satisfy request for intercept # iint
    }
    rpos = _intercepts[iint];
    _intercepts.erase(_intercepts.begin() + iint);
    // the above line is not as inefficient as an old C or early C++
    // programmer might think, due to the "allocator".

    local_to_global.ApplyPointTransform(rpos);  // convert to global coords

    // now apply material restriction, if any
    if (_material == 0) break;  // no material restriction

    G4VPhysicalVolume *pvtest = gNavigator->LocateGlobalPointAndSetup(rpos, 0, true);  // fast check mode
    if (pvtest != 0 && pvtest->GetLogicalVolume()->GetMaterial() == _material) break;  // we found it!
  }

  argResult = rpos;
}

void GLG4PosGen_Paint::SetState(G4String newValues) {
  newValues = util_strip_default(newValues);
  if (newValues.length() == 0) {
    // print help and current state
    RAT::info << "Current state of this GLG4PosGen_Paint:\n"
              << " \"" << GetState() << "\"\n"
              << newline;
    RAT::info << "Format of argument to GLG4PosGen_Paint::SetState: \n"
                 " \"x_mm y_mm z_mm [volumeName [thickness [materialName]]]\"\n"
                 " where x_mm, y_mm, z_mm are the coordinates (in mm) of a point,\n"
                 " volumeName is the name of the physicalVolume expected at the "
                 "point,\n"
                 " thickness is the thickness (in mm) of the layer of paint, and\n"
                 " materialName is an optional modifier which, if present, "
                 "restricts\n"
                 " points to daughter or sibling volumes in the selected region "
                 "which\n"
                 " are composed of the given material."
              << newline;
    return;
  }

  std::istringstream is(newValues.c_str());

  // set position
  G4double x, y, z;
  is >> x >> y >> z;
  if (is.fail()) {
    RAT::warn << "GLG4PosGen_Paint::SetState: "
                 "Could not parse three floats from input std::string"
              << newline;
    return;
  }
  _pos = G4ThreeVector(x, y, z);
  _pVolume = 0;
  _pVolumeName = "!";
  _thickness = 0.0;
  _materialName = "";
  _material = 0;
  _ntried = _nfound = 0;
  _intercepts.clear();

  is >> _pVolumeName >> _thickness >> _materialName;
  if (_pVolumeName.length() == 0) _pVolumeName = "!";
}

G4String GLG4PosGen_Paint::GetState() const {
  std::ostringstream os;
  G4String volname(_pVolumeName);

  if (_pVolumeName.length() == 0) {
    RAT::warn << "Warning: zero-length volume name caught in "
              << "GLG4PosGen_Paint::GetState()" << newline;
    volname = "!";
  }

  os << _pos.x() << ' ' << _pos.y() << ' ' << _pos.z() << ' ' << volname << ' ' << _thickness << ' ' << _materialName
     << std::ends;
  G4String rv(os.str());
  return rv;
}

////////////////////////////////////////////////////////////////

GLG4PosGen_Fill::GLG4PosGen_Fill(const char *arg_dbname)
    : GLG4PosGen(arg_dbname),
      _pos(0., 0., 0.),
      _pVolumeName("!"),
      _pVolume(0),
      _ntried(0),
      _nfound(0),
      _boundingBoxVolume(0.0) {}

void GLG4PosGen_Fill::GeneratePosition(G4ThreeVector &argResult) {
  // We need to refer to physical volume.
  // first set up the navigator to know the surrounding volume and
  // heirarchy (Note this must always be done, even if we already
  // know _pVolume.)
  G4VPhysicalVolume *pv;  // local variable for speed and later use
  G4Navigator *gNavigator = G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking();
  pv = gNavigator->LocateGlobalPointAndSetup(_pos, 0, false);  // full setup
  if (pv == 0) {
    G4cerr << "GLG4PosGen_Fill: "
              "Could not find any volume at "
           << _pos << newline;
    return;
  }

  // check or set related field variables
  if (pv != _pVolume) {   // normally happens just once after a SetState()
    if (_pVolume == 0) {  // SetState() sets _pVolume==0
      _pVolume = pv;
      if (_pVolumeName.length() == 0 || _pVolumeName == "!") _pVolumeName = _pVolume->GetName();
      if (_pVolumeName != _pVolume->GetName()) {
        G4cerr << "Warning: actual volume at " << _pos << " is " << _pVolume->GetName()
               << ", not equal to expected volume " << _pVolumeName << " in GLG4PosGen_Fill." << newline;
      }
    } else {
      G4cerr << "Warning: actual volume at " << _pos << " has changed! Now " << pv->GetName() << ", was "
             << _pVolume->GetName() << " in GLG4PosGen_PointPaintFill." << newline;
      _pVolume = pv;
      _pVolumeName = pv->GetName();
    }
    if (_materialName.length() > 0 && _material == 0) {
      _material = G4Material::GetMaterial(_materialName);
      if (_material == 0) {
        RAT::warn << "ERROR from GLG4PosGen_PointPaintFill: no material named " << _materialName
                  << ", material restriction cancelled." << newline;
        _materialName = "";
      }
    }
    _boundingBoxVolume = -1.0;
  }

  // here are some more things that both Paint and Fill algorithms need
  G4VSolid *solid = pv->GetLogicalVolume()->GetSolid();                         // this is the solid
  G4AffineTransform local_to_global = gNavigator->GetLocalToGlobalTransform();  // note Navigator init'ed above
  G4double x0, dx, y0, dy, z0, dz;                                              // for bounding box of solid
  {
    // get bounding box
    G4VoxelLimits voxelLimits;          // Defaults to "infinite" limits.
    G4AffineTransform affineTransform;  // Defaults to no transform
    G4double tmax;
    solid->CalculateExtent(kXAxis, voxelLimits, affineTransform, x0, tmax);
    dx = tmax - x0;
    solid->CalculateExtent(kYAxis, voxelLimits, affineTransform, y0, tmax);
    dy = tmax - y0;
    solid->CalculateExtent(kZAxis, voxelLimits, affineTransform, z0, tmax);
    dz = tmax - z0;
    if (_boundingBoxVolume <= 0.0)
      _boundingBoxVolume = dx * dy * dz;
    else if (fabs(dx * dy * dz / _boundingBoxVolume - 1.0) > 1e-6) {  // paranoia check
      RAT::warn << "Warning: volume of bounding box changed for " << _pVolumeName << " -- was " << _boundingBoxVolume
                << ", now " << dx * dy * dz << ", fractional change " << fabs(dx * dy * dz / _boundingBoxVolume - 1.0)
                << " -- PARANOIA IS JUSTIFIED!!!!" << newline;
      _boundingBoxVolume = dx * dy * dz;
    }
  }

  // more complicated case: generate points uniformly in the
  // surrounding volume.

  // loop over the following:
  //  generate points in bounding box of solid until we find one inside
  //  convert to global coordinates
  //  if _material == 0, then
  //    locate global point and see if it is in target physical volume
  //      (N.B. it might legitimately be in a daughter volume)
  //  if _material != 0, then
  //    locate global point and see if the corresponding volume
  //    is made of the stated material (ok if it is a daughter volume)
  // repeat until point found in target volume
  G4ThreeVector rpos;  // this will hold the position result
  // look for internal point
  for (int iloop = 0, jloop = 0; /* test inside */; iloop++) {
    do {
      rpos = G4ThreeVector(x0 + dx * G4UniformRand(), y0 + dy * G4UniformRand(),
                           z0 + dz * G4UniformRand());  // uniform in bounding box
      jloop++;
      if (jloop >= 100000) {
        RAT::warn << "GLG4PosGen_PointPaintFill::GeneratePosition(): " << iloop << "," << jloop
                  << " loops spent looking for point in " << _pVolumeName;
        if (_material) RAT::warn << " with material " << _materialName;
        RAT::warn << newline;
        argResult = rpos;
        return;
      }
      _ntried++;
    } while (!solid->Inside(rpos));
    local_to_global.ApplyPointTransform(rpos);                                         // convert to global coords
    G4VPhysicalVolume *pvtest = gNavigator->LocateGlobalPointAndSetup(rpos, 0, true);  // fast check mode
    if (_material == 0) {
      if (pvtest == pv) break;  // we found it!
    } else {
      if (pvtest != 0 && pvtest->GetLogicalVolume()->GetMaterial() == _material) break;  // we found it!
    }
  }
  _nfound++;

  argResult = rpos;
  return;
}

void GLG4PosGen_Fill::SetState(G4String newValues) {
  newValues = util_strip_default(newValues);
  if (newValues.length() == 0) {
    // print help and current state
    RAT::info << "Current state of this GLG4PosGen_PointPaintFill:\n"
              << " \"" << GetState() << "\"\n"
              << newline;
    RAT::info << "Format of argument to GLG4PosGen_PointPaintFill::SetState: \n"
                 " \"x_mm y_mm z_mm [fill [volumeName [materialName]]]\"\n"
                 " or\n"
                 " where x_mm, y_mm, z_mm are the coordinates (in mm) of a point,\n"
                 " volumeName is the name of the physicalVolume expected at the "
                 "point,\n"
                 " thickness is the thickness (in mm) of the layer of paint, and\n"
                 " materialName is an optional modifier which, if present, "
                 "restricts\n"
                 " points to daughter or sibling volumes in the selected region "
                 "which\n"
                 " are composed of the given material."
              << newline;
    return;
  }
  if (newValues == "volume?") {
    if (_ntried <= 0) {
      RAT::info << "volume? inquiry can only be used after filling." << newline;
    } else {
      RAT::info << "Fill volume information for " << _pVolumeName << ":" << _materialName << newline;
      RAT::info << "  ntried= " << _ntried << newline;
      RAT::info << "  nfound= " << _nfound << newline;
      RAT::info << "  bounding box volume: " << _boundingBoxVolume / CLHEP::meter3 << " m^3\n";
      RAT::info << "  filled volume: " << _boundingBoxVolume * _nfound / (double)_ntried / CLHEP::meter3 << " m^3\n";
      RAT::info << "  est. fractional precision: " << sqrt((_ntried - _nfound) * (double)_nfound / _ntried) / _ntried
                << newline;
    }
    return;
  }

  std::istringstream is(newValues.c_str());

  // set position
  G4double x, y, z;
  is >> x >> y >> z;
  if (is.fail()) {
    RAT::warn << "GLG4PosGen_PointPaintFill::SetState: "
                 "Could not parse three floats from input std::string"
              << newline;
    return;
  }
  _pos = G4ThreeVector(x, y, z);
  _pVolume = 0;
  _pVolumeName = "!";
  _materialName = "";
  _material = 0;
  _ntried = _nfound = 0;

  is >> _pVolumeName >> _materialName;
  // any failure can be safely ignored
}

G4String GLG4PosGen_Fill::GetState() const {
  std::ostringstream os;
  G4String volname(_pVolumeName);

  if (_pVolumeName.length() == 0) {
    RAT::warn << "Warning: zero-length volume name caught in "
              << "GLG4PosGen_FilL::GetState()" << newline;
    volname = "!";
  }

  os << _pos.x() << ' ' << _pos.y() << ' ' << _pos.z() << ' ' << volname << ' ' << _materialName;

  os << std::ends;
  G4String rv(os.str());
  return rv;
}

GLG4PosGen_FillCyl::GLG4PosGen_FillCyl(const char *arg_dbname)
    : GLG4PosGen(arg_dbname), _radius(0), _height(0), _volumeInfoLoaded(0) {}

void GLG4PosGen_FillCyl::GeneratePosition(G4ThreeVector &argResult) {
  double z = G4UniformRand() * (2 * _height) - _height;
  double r = sqrt(G4UniformRand() * _radius * _radius);
  double t = 2 * G4UniformRand() * 3.14159265;
  double x = r * cos(t);
  double y = r * sin(t);
  argResult = G4ThreeVector(x, y, z);

  // if (!_volumeInfoLoaded) {
  //   G4Navigator *theNavigator = G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking();
  //   G4VPhysicalVolume *thePhyVolume = theNavigator->LocateGlobalPointAndSetup(argResult);
  //   G4LogicalVolume *theLogVolume = thePhyVolume->GetLogicalVolume();
  //   G4Material *theMaterial = theLogVolume->GetMaterial();
  //   G4VSolid *theSolid = theLogVolume->GetSolid();
  //   double theVolume = theSolid->GetCubicVolume();  // in cubic meters
  //   double theMass = theLogVolume->GetMass(false, false);
  //   double theNelec = theMaterial->GetElectronDensity();
  //   RAT::debug << "[GLG4PosGen_FillCyl] Volume " << theVolume << " " << theMass << newline;
  //   RAT::debug << "[GLG4PosGen_FillCyl] Nelec " << theNelec << newline;
  // }
  return;
}

void GLG4PosGen_FillCyl::SetState(G4String newValues) {
  newValues = util_strip_default(newValues);
  if (newValues.length() == 0) {
    // print help and current state
    RAT::info << "Current state of this GLG4PosGen_FillCyl:\n"
              << " \"" << GetState() << "\"\n"
              << newline;
    RAT::info << "Format of argument to GLG4PosGen_FillCyl::SetState: \n"
                 " height_mm radius_mm\n"
              << newline;
    return;
  }

  std::istringstream is(newValues.c_str());

  // set position
  G4double height, radius;
  is >> height >> radius;
  if (is.fail()) {
    RAT::warn << "GLG4PosGen_FillCyl::SetState: "
                 "Could not parse two floats from input std::string"
              << newline;
    return;
  }
  _height = height;
  _radius = radius;
}

G4String GLG4PosGen_FillCyl::GetState() const {
  std::ostringstream os;

  os << _height << ' ' << _radius;

  os << std::ends;
  G4String rv(os.str());
  return rv;
}

////////////////////////////////////////////////////////////////

// FIXME: Need to figure out what to do with this

#if 0

GLG4PosGen_Cosmic::GLG4PosGen_Cosmic(const char *arg_dbname)
  :   GLG4PosGen(arg_dbname),
      _width(0.0), _height(0.0)
{
}

void GLG4PosGen_Cosmic::GenerateVertexPositions( G4PrimaryVertex *argVertex,
					 double max_chain_time,
					 double /*event_rate*/,
					 double dt
					)
{
  // find first legitimate primary particle in vertex (skip over informatons)
  G4PrimaryParticle* pp= argVertex->GetPrimary(0);
  while ( pp!=NULL &&
	  abs(pp->GetPDGcode())
	  >= ( GLG4VertexGen_HEPEvt::kPDGcodeModulus
	       * GLG4VertexGen_HEPEvt::kISTHEP_InformatonMin ) )
    // this particle had ISTHEP >= 100, it is an informaton
    pp= pp->GetNext();

  if (pp == NULL) {
    RAT::warn << "Error in GLG4PosGen_Cosmic::GenerateVertexPositions: "
	   << "no primary track in vertex!\n";
    return;
  }

  // generate orthogonal unit std::vectors to incident direction
  G4ThreeVector dir( pp->GetMomentum().unit() );
  if (dir.x() == 0.0 && dir.y() == 0.0 && dir.z() == 0.0) {
    RAT::warn << "Error in GLG4PosGen_Cosmic::GenerateVertexPositions: "
	   << "primary track has zero momentum!  I don't know what to do!\n";
    return;
  }
  G4ThreeVector e1(dir.y(), -dir.x(), 0.0);
  G4double tmp= e1.mag2();
  if (tmp == 0.0)
    e1.setX(1.0);
  else
    e1*= 1.0/sqrt(tmp);
  G4ThreeVector e2(dir.cross(e1).unit());

  // generate position in rectangle normal to incident direction,
  // offset a suitable distance back along direction from origin outside world
  G4ThreeVector startPos( e1*(_width*(G4UniformRand()-0.5))
			  +e2*(_height*(G4UniformRand()-0.5))
			  -dir*(_width+_height) );

  // find entrance point to Geant4 world
  G4Navigator* gNavigator =
    G4TransportationManager::GetTransportationManager()
    ->GetNavigatorForTracking();
  G4VSolid* worldSolid=
    gNavigator->GetWorldVolume()->GetLogicalVolume()->GetSolid();
  G4double dist_to_in=
    worldSolid->DistanceToIn(startPos, dir);

  if (dist_to_in < kInfinity) {
    // this track hits the world, so set the vertices there
    startPos += dist_to_in * dir;
    for (G4PrimaryVertex* v= argVertex; v!=NULL; v=v->GetNext()) {
      if (v->GetT0() > max_chain_time) {   // reached clip point of chain?
	RAT::warn << "GLG4PosGen_Cosmic::GenerateVertexPositions: Warning, "
	  "vertex time exceeds clip, but splitting not supported for cosmics\n"
	  "\t t0=" << v->GetT0() << " max_chain_time=" << max_chain_time
	       << newline;
      }
      v->SetPosition( v->GetX0() + startPos.x(),
		      v->GetY0() + startPos.y(),
		      v->GetZ0() + startPos.z() );
      v->SetT0( v->GetT0() + dt );
      // just before we are done, reset navigator, to avoid bugs
      // that confuse muon tracking
      gNavigator->LocateGlobalPointWithinVolume(startPos);
    }
  }
  else {
    // this track misses the world, so set an impossible effective ISTHEP
    // so Geant4 doesn't try to track them
    for (G4PrimaryVertex* v= argVertex; v!=NULL; v=v->GetNext()) {
      for (G4PrimaryParticle* pp= v->GetPrimary(0);
	   pp!=NULL;  pp=pp->GetNext() ) {
	if (abs(pp->GetPDGcode()) < GLG4VertexGen_HEPEvt::kPDGcodeModulus )
	  pp->SetPDGcode(pp->GetPDGcode() < 0 ?
			 pp->GetPDGcode()-GLG4VertexGen_HEPEvt::kPDGcodeModulus:
			 pp->GetPDGcode()+GLG4VertexGen_HEPEvt::kPDGcodeModulus);
      }
    }
  }

}

void GLG4PosGen_Cosmic::GeneratePosition(G4ThreeVector *)
{
  RAT::warn << "GLG4PosGen_Cosmic::GeneratePosition: ERROR!!! "
    "this function should never be called.  Results undefined." << newline;
}

void GLG4PosGen_Cosmic::SetState( G4String newValues )
{
  Strip(newValues);
  if (newValues.length() == 0) {
    // print help and current state
    RAT::info << "Current state of this GLG4PosGen_Cosmic:\n"
	   << " \"" << GetState() << "\"\n" << newline;
    RAT::info << "Format of argument to GLG4PosGen_Cosmic::SetState: \n"
      " \"width_mm height_mm\"\n"
      " width_mm  == width of rectangular area normal to incident direction\n"
      " height_mm == height of rectangular area normal to incident direction\n"
      "See comments in header file for details.\n"
	   << newline;
    return;
  }

  std::istringstream is(newValues.c_str());

  // set width and height
  is >> _width >> _height;
  GLG4param &db ( GLG4param::GetDB() );
  db[ (_dbname+".width").c_str() ]= _width;
  db[ (_dbname+".height").c_str() ]= _height;
  if (is.fail()) {
    RAT::warn << "GLG4PosGen_Cosmic::SetState: "
      "Could not parse two floats from input std::string" << newline;
    return;
  }
}

G4String GLG4PosGen_Cosmic::GetState()
{
  std::ostringstream os;

  os << _width << ' ' << _height << std::ends;
  G4String rv(os.str());
  os.freeze(0); // avoid memory leak!
  return rv;
}

#endif
