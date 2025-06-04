###########################################################
# CMakeLists for geo module

add_library(geo OBJECT
        src/BWVetGenericChamber.cc
        src/BWVetGenericChamberHit.cc
        src/ConeWaveguideConstruction.cc
        src/ConeWaveguideFactory.cc
        src/DetectorConstruction.cc
        src/DetectorFactory.cc
        src/GLG4BoxSD.cc
        src/GLG4PMTSD.cc
        src/GLG4TestSolid.cc
        src/GLG4TorusStack.cc
        src/GeoBoxFactory.cc
        src/GeoBubbleFactory.cc
        src/GeoBuilder.cc
        src/GeoCalibrationStickFactory.cc
        src/GeoCherenkovSourceFactory.cc
        src/GeoConvexLensFactory.cc
        src/GeoFiberSensitiveDetector.cc
        src/GeoFiberSensitiveDetectorHit.cc
        src/GeoCutTubeFactory.cc
        src/GeoEosFactory.cc
        src/GeoFactory.cc
        src/GeoLensFactory.cc
        src/GeoPerfBoxFactory.cc
        src/GeoPerfSphereFactory.cc
        src/GeoPerfTubeFactory.cc
        src/GeoPolyArrayFactory.cc
        src/GeoPolygonFactory.cc
        src/GeoReflectorFactory.cc
        src/GeoReflectorWaveguideFactory.cc
        src/GeoRevArrayFactory.cc
        src/GeoRevolutionChimneyFactory.cc
        src/GeoRevolutionFactory.cc
        src/GeoSolidArrayFactoryBase.cc
        src/GeoNestedSolidArrayFactoryBase.cc
        src/GeoNestedTubeArrayFactory.cc
        src/GeoNestedTubeConstruction.cc
        src/GeoSolidFactory.cc
        src/GeoSphereFactory.cc
        src/GeoSurfaceFactory.cc
        src/GeoTorusFactory.cc
        src/GeoTubeArrayFactory.cc
        src/GeoTubeFactory.cc
        src/GeoTubeIntersectionFactory.cc
        src/GeoWaterBoxArrayFactory.cc
        src/Materials.cc
        src/TubeFacetSolid.cc
        src/UnionSolidArray.cc
        src/WaterBoxConstruction.cc
        src/WLSPFactory.cc
        src/WLSPCoverFactory.cc
        src/GDMLWriteStructure.cc
        src/pmt/CubicPMTConstruction.cc
        src/pmt/LAPPDConstruction.cc
        src/pmt/PMTArrayFactory.cc
        src/pmt/PMTConcentrator.cc
        src/pmt/PMTConstruction.cc
        src/pmt/PMTCoverageFactory.cc
        src/pmt/PMTFactoryBase.cc
        src/pmt/PMTInfoParser.cc
        src/pmt/RevolutionPMTConstruction.cc
        src/pmt/ToroidalPMTConstruction.cc
        src/pmt/CylindricalPMTConstruction.cc
        src/pmt/PMTEncapsulation.cc 
        src/pmt/HemisphereEncapsulation.cc
        )

# Set our include directories
target_include_directories(geo SYSTEM PUBLIC 
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}/stlplus>)
target_include_directories(geo PUBLIC
        $<BUILD_INTERFACE:${RATPAC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)

# Copy our headers when installing
file(COPY include/ DESTINATION ${RATPAC_INCLUDE_DIR})
