/**
 * @brief  Declaration of functions in the ANNRIGd_GeneratorConfigurator
 *         namespace.
 * @author Sebastian Lorenz
 * @date   2017-09-05
 */

#ifndef ANNRIGD_GENERATORCONFIGURATOR_HH_
#define ANNRIGD_GENERATORCONFIGURATOR_HH_

//==============================================================================
// INCLUDES

// STD includes
#include <string>

//==============================================================================
// FORWARD DECLARATIONS

namespace ANNRIGdGammaSpecModel {
class ANNRIGd_GdNCaptureGammaGenerator;
}

//==============================================================================
// FUNCTION DECLARATIONS

namespace ANNRIGdGammaSpecModel {
namespace ANNRIGd_GeneratorConfigurator {

void Configure(ANNRIGd_GdNCaptureGammaGenerator& generator, int captureID, int cascadeID,
               const std::string& gd156ContDatFileName, const std::string& gd158ContDatFileName);

} /* namespace ANNRIGd_GeneratorConfigurator */
} /* namespace ANNRIGdGammaSpecModel */

#endif /* ANNRIGD_GENERATORCONFIGURATOR_HH_ */
