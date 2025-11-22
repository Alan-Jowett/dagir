// SPDX-License-Identifier: MIT
// Provide Catch2 StringMaker for std::string_view to avoid linker errors
// Single TU implementation: only define when Catch2 hasn't provided it.

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>

// Define the specialization only when Catch2 does not already provide it.
#if !defined(CATCH_CONFIG_CPP17_STRING_VIEW) && !defined(CATCH_CONFIG_STRINGMAKER_HAS_STRING_VIEW)
namespace Catch {
std::string StringMaker<std::string_view>::convert(std::string_view sv) {
  return std::string(sv);
}

std::string StringMaker<std::string_view, void>::convert(std::string_view sv) {
  return StringMaker<std::string_view>::convert(sv);
}
}  // namespace Catch
#endif
// SPDX-License-Identifier: MIT
// Provide Catch2 StringMaker for std::string_view to avoid linker errors
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>

// Catch2 may already provide a StringMaker for std::string_view depending
// on the version and build options. Only define it here when Catch2 hasn't
// provided one to avoid duplicate-symbol linker errors on CI.
#if !defined(CATCH_CONFIG_CPP17_STRING_VIEW) && !defined(CATCH_CONFIG_STRINGMAKER_HAS_STRING_VIEW)
namespace Catch {
std::string StringMaker<std::string_view>::convert(std::string_view sv) {
  return std::string(sv);
}
}  // namespace Catch
#endif
