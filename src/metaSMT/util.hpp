#pragma once
#include <string>

namespace metaSMT {
  inline std::string toupper(std::string str) {
    for (auto& c : str) {
      c = std::toupper(c);
    }
    return str;
  }
}  // namespace metaSMT

//  vim: ft=cpp:ts=2:sw=2:expandtab
