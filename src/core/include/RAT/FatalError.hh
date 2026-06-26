/** @class FatalError
 *  Exception thrown by Log::Die() and a failed Log::Assert().
 *
 *  Throwing (rather than calling std::exit) lets the C++ stack unwind so
 *  destructors run, and allows a hosting interpreter such as ROOT's Cling
 *  to catch the error and return to its prompt instead of killing the
 *  interactive session.  Batch executables should catch this in main() and
 *  return @c return_code so the process exit code is preserved.
 */

#ifndef __RAT_FatalError__
#define __RAT_FatalError__

#include <stdexcept>
#include <string>

namespace RAT {

class FatalError : public std::runtime_error {
 public:
  FatalError(const std::string &message, int return_code = 1) : std::runtime_error(message), return_code(return_code) {}
  int return_code; /**< Exit code originally passed to Die()/Assert(). */
};

}  // namespace RAT

#endif
