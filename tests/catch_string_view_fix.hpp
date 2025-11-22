// SPDX-License-Identifier: MIT
// Provide Catch2 StringMaker for std::string_view to avoid linker errors
// This header defines the specialization inline/inline-local to avoid ODR
// violations when Catch2 already provides the same symbol in the test
// framework static library.

#pragma once

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>

// Only define our small helper if Catch2 hasn't provided it.
#if !defined(CATCH_CONFIG_CPP17_STRING_VIEW) && !defined(CATCH_CONFIG_STRINGMAKER_HAS_STRING_VIEW)
namespace Catch {

// Make the function `static inline` via an inline method on a helper struct
// to ensure internal linkage / inline semantics and avoid ODR issues.
struct __catch_string_view_stringifier {
  static inline std::string convert(std::string_view sv) { return std::string(sv); }
};

template <>
struct StringMaker<std::string_view> {
  static std::string convert(std::string_view sv) {
    return __catch_string_view_stringifier::convert(sv);
  }
};

}  // namespace Catch
#endif
