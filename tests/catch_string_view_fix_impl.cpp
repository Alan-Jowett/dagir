// SPDX-License-Identifier: MIT
// Implementation TU to ensure a single definition of Catch::StringMaker<std::string_view>::convert
// This file defines the symbol only when Catch2 doesn't provide it (guarded), so it avoids ODR
// conflicts on CI where Catch2 may already supply the definition.

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>

// Only provide the missing symbol on Windows builds where Catch2 may not
// provide it but test code expects it. On Linux CI we avoid defining this
// symbol to prevent duplicate-definition errors when Catch2 supplies it.
#if (defined(_WIN32) || defined(_WIN64)) && !defined(CATCH_CONFIG_CPP17_STRING_VIEW) && \
    !defined(CATCH_CONFIG_STRINGMAKER_HAS_STRING_VIEW)
namespace Catch {
std::string StringMaker<std::string_view>::convert(std::string_view sv) {
  return std::string(sv);
}
}  // namespace Catch
#endif
