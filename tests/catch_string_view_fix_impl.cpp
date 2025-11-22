// SPDX-License-Identifier: MIT
// Implementation TU to ensure a single definition of Catch::StringMaker<std::string_view>::convert
// This file defines the symbol only when Catch2 doesn't provide it (guarded), so it avoids ODR
// conflicts on CI where Catch2 may already supply the definition.

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>

#if (defined(_WIN32) || defined(_WIN64)) && !defined(CATCH_CONFIG_STRINGMAKER_HAS_STRING_VIEW)
namespace Catch {
std::string StringMaker<std::string_view>::convert(std::string_view sv) {
  return std::string(sv);
}

// Some Catch headers/libraries may reference the two-parameter variant
// `StringMaker<T,void>`. Provide that symbol as an alias to the single-
// parameter implementation to cover both cases.
std::string StringMaker<std::string_view, void>::convert(std::string_view sv) {
  return StringMaker<std::string_view>::convert(sv);
}

}  // namespace Catch
#endif
