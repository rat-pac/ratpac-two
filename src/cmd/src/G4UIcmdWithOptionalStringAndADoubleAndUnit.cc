#include <G4Tokenizer.hh>
#include <G4UIcommandStatus.hh>
#include <G4UnitsTable.hh>
#include <RAT/G4UIcmdWithOptionalStringAndADoubleAndUnit.hh>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace RAT {

namespace {

constexpr const char *kNoSelector = "__none__";

bool IsDouble(const G4String &value) {
  std::istringstream input(value);
  double parsed;
  input >> parsed;
  return input && input.peek() == std::char_traits<char>::eof();
}

}  // namespace

G4UIcmdWithOptionalStringAndADoubleAndUnit::G4UIcmdWithOptionalStringAndADoubleAndUnit(const char *commandPath,
                                                                                       G4UImessenger *messenger,
                                                                                       const char *valueName,
                                                                                       const char *defaultUnit,
                                                                                       const char *selectorName)
    : G4UIcommand(commandPath, messenger), fDefaultUnit(defaultUnit) {
  if (!G4UnitDefinition::IsUnitDefined(defaultUnit)) {
    throw std::invalid_argument("Unknown default unit: " + std::string(defaultUnit));
  }
  fUnitCategory = CategoryOf(defaultUnit);
  const G4String firstParameterName = G4String(selectorName) + "Or" + valueName;
  const G4String secondParameterName = G4String(valueName) + "OrUnit";
  SetParameter(new G4UIparameter(firstParameterName.c_str(), 's', false));
  SetParameter(new G4UIparameter(secondParameterName.c_str(), 's', false));
  G4UIparameter *unitParameter = new G4UIparameter("unit", 's', true);
  unitParameter->SetDefaultValue(kNoSelector);
  SetParameter(unitParameter);
}

G4int G4UIcmdWithOptionalStringAndADoubleAndUnit::DoIt(const G4String &parameterList) {
  try {
    Parse(parameterList, false);
  } catch (const std::invalid_argument &error) {
    G4ExceptionDescription description;
    description << error.what();
    CommandFailed(description);
    return fParameterUnreadable;
  }
  return G4UIcommand::DoIt(parameterList);
}

G4UIcmdWithOptionalStringAndADoubleAndUnit::Value G4UIcmdWithOptionalStringAndADoubleAndUnit::GetNewValue(
    const G4String &parameterList) const {
  return Parse(parameterList, true);
}

G4String G4UIcmdWithOptionalStringAndADoubleAndUnit::ConvertToString(G4double value) const {
  return G4UIcommand::ConvertToString(value, fDefaultUnit);
}

G4String G4UIcmdWithOptionalStringAndADoubleAndUnit::ConvertToString(const G4String &selector, G4double value) const {
  return selector + " " + ConvertToString(value);
}

G4UIcmdWithOptionalStringAndADoubleAndUnit::Value G4UIcmdWithOptionalStringAndADoubleAndUnit::Parse(
    const G4String &parameterList, bool canonical) const {
  std::vector<G4String> arguments;
  G4Tokenizer tokenizer(parameterList);
  G4String argument;
  while (!(argument = tokenizer()).empty()) arguments.push_back(argument);

  Value result{false, "", 0.0};
  G4String value;
  G4String unit;
  if (arguments.size() == 2) {
    value = arguments[0];
    unit = arguments[1];
  } else if (arguments.size() == 3 && canonical && arguments[2] == kNoSelector) {
    value = arguments[0];
    unit = arguments[1];
  } else if (arguments.size() == 3) {
    result.hasSelector = true;
    result.selector = arguments[0];
    value = arguments[1];
    unit = arguments[2];
  } else {
    throw std::invalid_argument("Expected two or three parameters for " + GetCommandPath());
  }

  if (!IsDouble(value)) throw std::invalid_argument("Invalid numeric value: " + value);
  if (!G4UnitDefinition::IsUnitDefined(unit)) throw std::invalid_argument("Unknown unit: " + unit);
  if (CategoryOf(unit) != fUnitCategory) {
    throw std::invalid_argument("Unit " + unit + " is not in the " + fUnitCategory + " category");
  }
  const G4String dimensionedValue = value + " " + unit;
  result.value = G4UIcommand::ConvertToDimensionedDouble(dimensionedValue.c_str());
  return result;
}

}  // namespace RAT
