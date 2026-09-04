#ifndef __RAT_G4UIcmdWithOptionalStringAndADoubleAndUnit__
#define __RAT_G4UIcmdWithOptionalStringAndADoubleAndUnit__

#include <G4UIcommand.hh>

namespace RAT {

/** Geant4 command containing a dimensioned double and an optional leading string selector. */
class G4UIcmdWithOptionalStringAndADoubleAndUnit : public G4UIcommand {
 public:
  struct Value {
    bool hasSelector;
    G4String selector;
    G4double value;
  };

  G4UIcmdWithOptionalStringAndADoubleAndUnit(const char *commandPath, G4UImessenger *messenger, const char *valueName,
                                             const char *defaultUnit, const char *selectorName);

  G4int DoIt(const G4String &parameterList) override;

  /** Convert the parameter string delivered to the messenger into internal Geant4 units. */
  Value GetNewValue(const G4String &parameterList) const;

  /** Format a value in the command's default units. */
  G4String ConvertToString(G4double value) const;

  /** Format a selector and value in the command's default units. */
  G4String ConvertToString(const G4String &selector, G4double value) const;

 private:
  Value Parse(const G4String &parameterList, bool canonical) const;

  G4String fDefaultUnit;
  G4String fUnitCategory;
};

}  // namespace RAT

#endif
