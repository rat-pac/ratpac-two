/**
 * @brief  Implementation of functions in the ANNRIGd_GeneratorConfigurator
 *         namespace.
 * @author Sebastian Lorenz
 * @date   2017-09-05
 */

//==============================================================================
// INCLUDES

// ANNRIGdGammaSpecModel includes
#include "RAT/ANNRIGd_GeneratorConfigurator.hh"

#include "RAT/ANNRIGd_156GdContinuumModelV2.hh"
#include "RAT/ANNRIGd_156GdDiscreteModel.hh"
#include "RAT/ANNRIGd_158GdContinuumModelV2.hh"
#include "RAT/ANNRIGd_158GdDiscreteModel.hh"
#include "RAT/ANNRIGd_GdNCaptureGammaGenerator.hh"

namespace AGd = ANNRIGdGammaSpecModel;

//==============================================================================
// FUNCTION IMPLEMENTATIONS

namespace ANNRIGdGammaSpecModel {

//______________________________________________________________________________
/**
 * @brief Configures the given ANNRIGd_GdNCaptureGammaGenerator instance.
 *        The configuration process sets the necessary models based on the given
 *        values for the captureID and cascadeID.
 * @param generator ANNRIGd_GdNCaptureGammaGenerator instance to configure.
 * @param captureID  ID describing the capture process for which the gamma-rays
 *        shall be generated.
 *        - 1 : natGd(n,g) reaction; all models are required
 *        - 2 : 157Gd(n,g) reaction; required models depend on cascade ID value.
 *        - 3 : 155Gd(n,g) reaction; required models depend on cascade ID value.
 * @param cascadeID  ID describing the cascade type that shall be used to
 *        generate the gamma-rays.
 *        - 1 : both discrete and continuum spectrum components
 *        - 2 : discrete spectrum component
 *        - 3 : continuum spectrum component
 * @param gd156ContDatFileName  Name of file containing the necessary data to
 *        generate the 156Gd continuum spectrum component.
 * @param gd158ContDatFileName  Name of file containing the necessary data to
 *        generate the 158Gd continuum spectrum component.
 */
void ANNRIGd_GeneratorConfigurator::Configure(ANNRIGd_GdNCaptureGammaGenerator& generator, int captureID, int cascadeID,
                                              const std::string& gd156ContDatFileName,
                                              const std::string& gd158ContDatFileName) {
  // select which model is needed
  switch (captureID) {
    case 1:  // natGd(n,g) reaction - all models needed
      // models for 155Gd(n,g)
      generator.Set156GdContinuumModel(new AGd::ANNRIGd_156GdContinuumModelV2(gd156ContDatFileName));
      generator.Set156GdDiscreteModel(new AGd::ANNRIGd_156GdDiscreteModel());
      // models for 157Gd(n,g)
      generator.Set158GdContinuumModel(new AGd::ANNRIGd_158GdContinuumModelV2(gd158ContDatFileName));
      generator.Set158GdDiscreteModel(new AGd::ANNRIGd_158GdDiscreteModel());
      break;
    case 2:  // 157Gd(n,g) reaction
      switch (cascadeID) {
        case 1:  // discrete and continuum
          generator.Set158GdContinuumModel(new AGd::ANNRIGd_158GdContinuumModelV2(gd158ContDatFileName));
          generator.Set158GdDiscreteModel(new AGd::ANNRIGd_158GdDiscreteModel());
          break;
        case 2:  // discrete
          generator.Set158GdDiscreteModel(new AGd::ANNRIGd_158GdDiscreteModel());
          break;
        case 3:  // continuum
          generator.Set158GdContinuumModel(new AGd::ANNRIGd_158GdContinuumModelV2(gd158ContDatFileName));
          break;
        default:
          break;
      }
      break;
    case 3:  // 155Gd(n,g) reaction
      switch (cascadeID) {
        case 1:  // discrete and continuum
          generator.Set156GdContinuumModel(new AGd::ANNRIGd_156GdContinuumModelV2(gd156ContDatFileName));
          generator.Set156GdDiscreteModel(new AGd::ANNRIGd_156GdDiscreteModel());
          break;
        case 2:  // discrete
          generator.Set156GdDiscreteModel(new AGd::ANNRIGd_156GdDiscreteModel());
          break;
        case 3:  // continuum
          generator.Set156GdContinuumModel(new AGd::ANNRIGd_156GdContinuumModelV2(gd156ContDatFileName));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

} /* namespace ANNRIGdGammaSpecModel */
