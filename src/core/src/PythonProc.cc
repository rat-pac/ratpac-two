#include <RVersion.h>
#include <TPython.h>
#include <TRegexp.h>

#include <RAT/PythonProc.hh>

namespace RAT {

int PythonProc::fgProcCounter = 0;

PythonProc::PythonProc() : Processor("python") { fPyProcName = dformat("proc_%i", fgProcCounter++); }

PythonProc::~PythonProc() {
  std::string cmd = fPyProcName + ".finish()";
  TPython::Exec(cmd.c_str());
  cmd = "del " + fPyProcName;
  TPython::Exec(cmd.c_str());
}

void PythonProc::SetS(std::string param, std::string value) {
  if (param == "exec")
    TPython::Exec(value.c_str());
  else if (param == "class") {
    TRegexp is_identifier("^[a-zA-Z_][a-zA-Z0-9_]*$");

    std::vector<std::string> parts = split(value, ".");

    unsigned int module_parts = 0;
    while (module_parts < parts.size() - 1) {
      const std::string part = parts[module_parts];
      Ssiz_t len;
      if (is_identifier.Index(part, &len) > -1)
        module_parts++;
      else
        break;
    }

    unsigned int npop = parts.size() - module_parts;
    for (unsigned int i = 0; i < npop; i++) parts.pop_back();

    if (module_parts > 0) {
      std::string cmd = "import " + join(parts, ".");
      Log::Assert(TPython::Exec(cmd.c_str()), "Python exception");
    }

    // Add final paretheses if initialization arguments not given
    std::string cmd = fPyProcName + " = " + value;
    if (cmd[cmd.size() - 1] != ')') cmd += "()";

    TPython::Exec(cmd.c_str());
    // Update name of this processor to include Python class
    name = value;
  } else if (param == "init") {
    std::string cmd = fPyProcName + " = " + value;
    TPython::Exec(cmd.c_str());
    // Update name of this processor to include Python class
    name = value;
  }
}

#if ROOT_VERSION_CODE < ROOT_VERSION(6, 34, 0)
[[deprecated(
    "ROOT < 6.34: This implementation is outdated due to the deprecation of TPython::Eval(). Please upgrade to a newer "
    "version of ROOT.")]] Processor::Result
PythonProc::DSEvent(DS::Root *ds) {
  TPython::Bind(ds, "ds");
  std::string cmd = fPyProcName + ".dsevent(ds)";
  // First cast to int since that is supported by TPyResult
  int result = TPython::Eval(cmd.c_str());
  if (result == -1) Log::Die("Python exception.");
  return (Processor::Result)result;
}
#else
Processor::Result PythonProc::DSEvent(DS::Root *ds) {
  TPython::Bind(ds, "ds");
  std::string cmd = "_anyresult = " + fPyProcName + ".dsevent(ds)";
  std::any result;
  bool python_ok = TPython::Exec(cmd.c_str(), &result);
  if (!python_ok) Log::Die("Python exception.");
  return std::any_cast<Processor::Result>(result);
}

#endif

}  // namespace RAT
