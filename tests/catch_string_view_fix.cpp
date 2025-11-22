// SPDX-License-Identifier: MIT
// Provide Catch2 StringMaker for std::string_view to avoid linker errors
// Single TU implementation: only define when Catch2 hasn't provided it.

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>

// On Windows MSVC builds the two-parameter instantiation may be required
// but not provided by the Catch2 library; define it here to ensure the
// test binary links. On other platforms only define when Catch2 hasn't
// provided a stringifier to avoid ODR / duplicate-definition issues.
#if (defined(_WIN32) || defined(_WIN64)) && !defined(CATCH_CONFIG_STRINGMAKER_HAS_STRING_VIEW)
namespace Catch {
std::string StringMaker<std::string_view>::convert(std::string_view sv) {
  return std::string(sv);
}
}  // namespace Catch
#endif
