#include <G4SDManager.hh>
#include <RAT/ConeWaveguideFactory.hh>
#include <RAT/DetectorConstruction.hh>
#include <RAT/Factory.hh>
#include <RAT/GeoBoxFactory.hh>
#include <RAT/GeoBubbleFactory.hh>
#include <RAT/GeoBuilder.hh>
#include <RAT/GeoCalibrationStickFactory.hh>
#include <RAT/GeoCherenkovSourceFactory.hh>
#include <RAT/GeoConvexLensFactory.hh>
#include <RAT/GeoCutTubeFactory.hh>
#include <RAT/GeoLensFactory.hh>
#include <RAT/GeoNestedTubeArrayFactory.hh>
#include <RAT/GeoPerfBoxFactory.hh>
#include <RAT/GeoPerfSphereFactory.hh>
#include <RAT/GeoPerfTubeFactory.hh>
#include <RAT/GeoPolyArrayFactory.hh>
#include <RAT/GeoPolygonFactory.hh>
#include <RAT/GeoReflectorFactory.hh>
#include <RAT/GeoReflectorWaveguideFactory.hh>
#include <RAT/GeoRevArrayFactory.hh>
#include <RAT/GeoRevolutionChimneyFactory.hh>
#include <RAT/GeoRevolutionFactory.hh>
#include <RAT/GeoSphereFactory.hh>
#include <RAT/GeoSurfaceFactory.hh>
#include <RAT/GeoTorusFactory.hh>
#include <RAT/GeoTubeArrayFactory.hh>
#include <RAT/GeoTubeFactory.hh>
#include <RAT/GeoTubeIntersectionFactory.hh>
#include <RAT/GeoWaterBoxArrayFactory.hh>
#include <RAT/Log.hh>
#include <RAT/PMTArrayFactory.hh>
#include <RAT/PMTCoverageFactory.hh>
#include <RAT/WLSPCoverFactory.hh>
#include <RAT/WLSPFactory.hh>

namespace RAT {

GeoBuilder::GeoBuilder() {
  // Register all the standard volumes
  new GeoBoxFactory();
  new GeoTubeFactory();
  new GeoTorusFactory();
  new GeoSphereFactory();
  new GeoReflectorFactory();
  new GeoReflectorWaveguideFactory();
  new PMTArrayFactory();
  new GeoNestedTubeArrayFactory();
  new PMTCoverageFactory();
  new GeoWaterBoxArrayFactory();
  new GeoBubbleFactory();
  new GeoPerfTubeFactory();
  new GeoPerfSphereFactory();
  new GeoRevArrayFactory();
  new GeoTubeArrayFactory();
  new GeoRevolutionFactory();
  new GeoRevolutionChimneyFactory();
  new GeoLensFactory();
  new GeoPolygonFactory();
  new GeoSurfaceFactory();
  new GeoConvexLensFactory();
  new GeoTubeIntersectionFactory();
  new GeoPerfBoxFactory();
  new GeoCutTubeFactory();
  new GeoPolyArrayFactory();
  new WLSPFactory();
  new WLSPCoverFactory();

  // Extra components
  new GeoCherenkovSourceFactory();
  new GeoCalibrationStickFactory();

  // Register standard waveguides
  GlobalFactory<WaveguideFactory>::Register("cone", new Alloc<WaveguideFactory, ConeWaveguideFactory>);
}

G4VPhysicalVolume *GeoBuilder::ConstructAll(std::string geo_tablename) {
  // Get all geometry tables that have been loaded
  DBLinkGroup geo = DB::Get()->GetLinkGroup(geo_tablename);
  G4VPhysicalVolume *world = 0;

  // Plan: Scan list repeatedly, looking for volume with mother that
  //       already has been constructed.  This should reduce the size
  //       of geo by one each iteration if mother dependency is not
  //       circular.  (No Oedipus complexes allowed)  The case
  //       of circular dependency is detected and the program is aborted.
  //
  //       Also fail if you ever find a volume without an already built
  //       mother AND without a yet-to-be built mother.
  debug << "GeoBuilder: Starting ConstructAll()\n";

  while (geo.size() > 0) {
    DBLinkGroup::iterator i_table;
    for (i_table = geo.begin(); i_table != geo.end(); ++i_table) {
      std::string name = i_table->first;
      debug << "GeoBuilder: Checking " << name << newline;

      DBLinkPtr table = i_table->second;
      std::string mother;
      std::string type;
      try {
        mother = table->GetS("mother");
      } catch (DBNotFoundError &e) {
        Log::Die("GeoBuilder error: volume " + name + " has no mother");
      }
      try {
        type = table->GetS("type");
      } catch (DBNotFoundError &e) {
        Log::Die("GeoBuilder error: volume " + name + " has no type");
      }

      // Skip disabled volumes
      int enabled = 1;
      try {
        enabled = table->GetI("enable");
      } catch (DBNotFoundError &e) {
      };

      if (!enabled) {
        debug << "GeoBuilder: Removing " << name << " (disabled) from geo list.\n";
        geo.erase(i_table);
        break;
      }

      if (type == "border") {
        std::string volume1, volume2;
        try {
          volume1 = table->GetS("volume1");
        } catch (DBNotFoundError &e) {
          Log::Die("GeoBuilder error: border " + name + " has no volume1");
        }
        try {
          volume2 = table->GetS("volume2");
        } catch (DBNotFoundError &e) {
          Log::Die("GeoBuilder error: border " + name + " has no volume2");
        }
        G4LogicalVolume *LogVol1 = GeoFactory::FindMother(volume1);
        G4LogicalVolume *LogVol2 = GeoFactory::FindMother(volume2);

        if (LogVol1 != 0 && LogVol2 != 0) {
          try {
            GeoFactory::ConstructWithFactory(type, table);
          } catch (GeoFactoryNotFoundError &e) {
            Log::Die("GeoBuilder error: Cannot find factory for volume type " + type);
          }
          debug << "GeoBuilder: Removing " << name << " from geo list.\n";
          geo.erase(i_table);
          break;
        } else if ((LogVol1 == 0 && geo.count(volume1) == 0) || (LogVol2 == 0 && geo.count(volume2) == 0)) {
          // No mother yet to be built
          Log::Die("GeoBuilder error: Cannot find " + volume1 + " or " + volume2 + " for " + name);
        }
      } else {
        if (mother == "" || GeoFactory::FindMother(mother) != 0) {  // Found volume to build

          try {
            if (mother == "")
              world = GeoFactory::ConstructWithFactory(type, table);  // save world volume
            else
              GeoFactory::ConstructWithFactory(type, table);
          } catch (GeoFactoryNotFoundError &e) {
            Log::Die("GeoBuilder error: Cannot find factory for volume type " + type);
          }

          debug << "GeoBuilder: Removing " << name << " from geo list.\n";
          geo.erase(i_table);
          break;
        } else if (geo.count(mother) == 0) {  // No mother yet to be built
          Log::Die("GeoBuilder error: Cannot find mother volume " + mother + " for " + name);
        }
      }

    }  // end for loop looking for next volume to build

    // Circular dependency check
    if (i_table == geo.end()) {
      std::string err("GeoBuilder error: Circular volume dependency encountered!\n");
      for (i_table = geo.begin(); i_table != geo.end(); i_table++) {
        std::string name = i_table->first;
        DBLinkPtr table = i_table->second;
        err += "  " + name + " depends on " + table->GetS("mother") + "\n";
      }
      Log::Die(err);
    }

  }  // end loop when all volumes built

  if (world == 0) Log::Die("GeoBuilder error: No world volume defined");

  return world;
}

}  // namespace RAT
