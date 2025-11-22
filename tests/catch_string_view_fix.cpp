// SPDX-License-Identifier: MIT
// Provide Catch2 StringMaker for std::string_view to avoid linker errors
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>

namespace Catch {
#ifdef CATCH_CONFIG_CPP17_STRING_VIEW
std::string StringMaker<std::string_view>::convert(std::string_view sv) {
  return std::string(sv);
}
#endif
}  // namespace Catch
