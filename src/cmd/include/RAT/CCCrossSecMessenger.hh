////////////////////////////////////////////////////////////////////
///
/// \class RAT::CCCrossSecMessenger
///
/// \brief Messenger class to control cross section options.
///
/// \author Max Smiley <masmiley@berkeley.edu> -- contact person
///
/// \details Controls user customizable options for the cross section class.
///			At the moment the customizations are:
/// 		- Control over the weak mixing angle.
///			- Control over the cross section calculation strategy.
///\see RAT::ESCrossSec for more details.
///
///
////////////////////////////////////////////////////////////////////

#ifndef RAT_CCCrossSectionMessenger_hh
#define RAT_CCCrossSectionMessenger_hh

#include <G4String.hh>
#include <G4UImessenger.hh>

// Forward declarations
class G4UIcommand;
class G4UIcmdWithADouble;
class G4UIcmdWithAnInteger;

namespace RAT {

// Forward declarations in namespace
class CCCrossSec;

class CCCrossSecMessenger : public G4UImessenger {
 public:
  CCCrossSecMessenger(CCCrossSec *);
  ~CCCrossSecMessenger();

  void SetNewValue(G4UIcommand *command, G4String newValues);
  G4String GetCurrentValue(G4UIcommand *command);

 private:
  CCCrossSec *fCCXS;

  G4UIcmdWithADouble *fWmaCmd;
  G4UIcmdWithAnInteger *fStratCmd;
};

}  // namespace RAT

#endif  // RAT_CCCrossSecMessenger_hh
