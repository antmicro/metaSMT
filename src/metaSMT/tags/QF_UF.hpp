#pragma once
#ifndef HEADER_metaSMT_TAG_QF_UF_HPP
#define HEADER_metaSMT_TAG_QF_UF_HPP

#include <variant>

#include "../types/Types.hpp"
#include "Logic.hpp"

namespace metaSMT {
  namespace logic {
    namespace QF_UF {
      namespace tag {

#define PRINT(Tag, body)                                    \
  template <typename STREAM>                                \
  friend STREAM& operator<<(STREAM& out, Tag const& self) { \
    return (out << body);                                   \
  }

        // uninterpreted function variable tag
        struct function_var_tag {
          typedef attr::ignore attribute;

          unsigned id;
          type::any_type result_type;
          std::vector<type::any_type> args;

          PRINT(function_var_tag, "function_var_tag[" << self.id << "]")
          bool operator<(function_var_tag const& other) const { return id < other.id; }
        };

#undef PRINT

        using QF_UF_Tag = std::variant<nil, function_var_tag>;

      }  // namespace tag
    }    // namespace QF_UF
  }      // namespace logic
}  // namespace metaSMT
#endif  // HEADER_metaSMT_TAG_ARRAY_HPP
//  vim: ft=cpp:ts=2:sw=2:expandtab
