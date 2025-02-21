#pragma once
#ifndef HEADER_metaSMT_TAG_ARRAY_HPP
#define HEADER_metaSMT_TAG_ARRAY_HPP

#include "Logic.hpp"

namespace metaSMT {
  namespace logic {
    namespace Array {
      namespace tag {

#define PRINT(Tag, body)                                \
  template <typename STREAM>                            \
  friend STREAM &operator<<(STREAM &out, Tag const &) { \
    return (out << body);                               \
  }
#define TAG(NAME, ATTR)                                        \
  struct NAME##_tag {                                          \
    typedef attr::ATTR attribute;                              \
    bool operator<(NAME##_tag const &) const { return false; } \
    PRINT(NAME##_tag, #NAME)                                   \
  };

        // array variable tag
        struct array_var_tag {
          typedef attr::ignore attribute;

          unsigned id;
          unsigned elem_width;
          unsigned index_width;

          template <typename STREAM>
          friend STREAM &operator<<(STREAM &out, array_var_tag const &self) {
            return (out << "array_var_tag[" << self.id << ',' << self.elem_width << ',' << self.index_width << "]");
          }

          bool operator<(array_var_tag const &other) const { return id < other.id; }
        };

        TAG(select, ignore)
        TAG(store, ignore)

#undef PRINT
#undef TAG

        using Array_Tag = std::variant<nil, select_tag, store_tag, array_var_tag>;

      }  // namespace tag
    }    // namespace Array
  }      // namespace logic
}  // namespace metaSMT
#endif  // HEADER_metaSMT_TAG_ARRAY_HPP
//  vim: ft=cpp:ts=2:sw=2:expandtab
