#include <RAT/CCCrossSecMessenger.hh>
#include <RAT/CCCrossSec.hh>
#include <RAT/Log.hh>
#include <G4UIcommand.hh>
#include <G4UIdirectory.hh>
#include <G4UIcmdWithADouble.hh>
#include <G4UIcmdWithAnInteger.hh>
#include <G4String.hh>

namespace RAT {

  CCCrossSecMessenger::CCCrossSecMessenger(CCCrossSec* e) :
    fCCXS(e)
  {
    // Commands will go in a /generator/cc/ directory
    G4UIdirectory* dir = new G4UIdirectory("/generator/cc/xsection/");
    dir->SetGuidance("Control the physics parameters of the charged current generator");

    fWmaCmd = new G4UIcmdWithADouble("/generator/cc/xsection/wma", this);
    fWmaCmd->SetGuidance("Sets the value of sine-squared theta (the weak mixing angle)");
    fWmaCmd->SetParameterName("sin2th",false);


    fStratCmd = new G4UIcmdWithAnInteger("/generator/cc/xsection/strategy", this);
    fStratCmd->SetGuidance("Sets the strategy for the CC cross section calculation.");
    fStratCmd->SetGuidance("Usage: /generator/cc/xsection/strategy strat");
    fStratCmd->SetGuidance("Options:");
    fStratCmd->SetGuidance("  1 : Original routine from QSNO::PNuE (Bahcall).");
    fStratCmd->SetGuidance("  2 : Improved routine from QSNO::PNuE (without rad. corrections).");
    fStratCmd->SetGuidance("  3 : Improved routine from QSNO::PNuE (with rad. corrections - analytical).");
    fStratCmd->SetGuidance("  4 (default) : Improved routine from QSNO::PNuE (with rad. corrections - table).");
    fStratCmd->SetParameter(new G4UIparameter("strat",'i',false));


    //

  }

  CCCrossSecMessenger::~CCCrossSecMessenger() {;}

  void CCCrossSecMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
  {
    if ( command == fWmaCmd )
      {
        G4double wma = fWmaCmd->GetNewDoubleValue( newValue );
        std::cout << "Warning not using WMA: " << wma << std::endl;
      }
    else if ( command == fStratCmd )
      {
        G4int strat = fStratCmd->GetNewIntValue( newValue );
        std::cout << "Warning not using strategy: " << strat << std::endl;
      }
    else
      {
        warn << "Error: Invalid CCCrossSecMessenger \"set\" command" << newline;
      }
  }

  G4String CCCrossSecMessenger::GetCurrentValue(G4UIcommand* command)
  {
    //Dummy values for now
    if ( command == fWmaCmd )
      return fWmaCmd->ConvertToString(0);
    else if ( command == fStratCmd )
      return fStratCmd->ConvertToString(0);

    // Error if we reach here.
    return G4String("Error: Invalid CCCrossSecMessenger \"get\" command");
  }

} // namespace RAT
