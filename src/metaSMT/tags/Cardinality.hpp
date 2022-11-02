#pragma once

#include <variant>

#include "attribute.hpp"

namespace metaSMT {
  namespace logic {
    namespace cardinality {
      namespace tag {

#define PRINT(Tag, body)                                \
  template <typename STREAM>                            \
  friend STREAM &operator<<(STREAM &out, Tag const &) { \
    out << body;                                        \
    return out;                                         \
  }
#define TAG(NAME, ATTR)                                        \
  struct NAME##_tag {                                          \
    typedef attr::ATTR attribute;                              \
    bool operator<(NAME##_tag const &) const { return false; } \
    PRINT(NAME##_tag, #NAME)                                   \
  };

        TAG(lt, nary)
        TAG(le, nary)
        TAG(eq, nary)
        TAG(ge, nary)
        TAG(gt, nary)

#undef PRINT
#undef TAG

        // tag variant Cardinality_Tags
        using Cardinality_Tag = std::variant<lt_tag, le_tag, eq_tag, ge_tag, gt_tag>;
      }  // namespace tag
    }    // namespace cardinality
  }      // namespace logic
}  // namespace metaSMT

//  vim: ft=cpp:ts=2:sw=2:expandtab
