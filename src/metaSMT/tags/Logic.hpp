#pragma once

#include <variant>

#include "../impl/_var_id.hpp"
#include "attribute.hpp"

namespace metaSMT {
  struct nil {
    typedef attr::ignore attribute;

    bool operator<(nil const &) const { return false; };
    template <typename STREAM>
    friend STREAM &operator<<(STREAM &out, nil const &) {
      out << "nil";
      return out;
    }
  };

  namespace logic {
    namespace tag {

      // variable tag
      struct var_tag {
        unsigned id;
        typedef attr::ignore attribute;

        template <typename STREAM>
        friend STREAM &operator<<(STREAM &out, var_tag const &self) {
          return (out << "var_tag[" << self.id << "]");
        }

        bool operator<(var_tag const &other) const { return id < other.id; }
      };

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

      // constants
      TAG(true, constant)
      TAG(false, constant)
      TAG(bool, constant)

      // unary
      TAG(not, unary)

      // binary
      TAG(equal, binary)  // chainable
      TAG(nequal, binary)
      TAG(distinct, binary)  // pairwise
      TAG(implies, binary)   // right_assoc

      TAG(and, left_assoc)
      TAG(nand, binary)
      TAG(or, left_assoc)
      TAG(nor, binary)
      TAG(xor, binary)  // left_assoc
      TAG(xnor, binary)

      // ternary
      TAG(ite, ternary)

#undef PRINT
#undef TAG
      //
      // tag variant Predicate
      using Predicate_Tag = std::variant<false_tag, true_tag, not_tag, equal_tag, nequal_tag, distinct_tag, and_tag,
                                         nand_tag, or_tag, nor_tag, xor_tag, xnor_tag, implies_tag, ite_tag, var_tag>;

    }  // namespace tag
    inline tag::var_tag new_variable() {
      tag::var_tag tag;
      tag.id = impl::new_var_id();
      return tag;
    }
  }  // namespace logic
}  // namespace metaSMT

//  vim: ft=cpp:ts=2:sw=2:expandtab
