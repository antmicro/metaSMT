#pragma once
#ifndef HEADER_metaSMT_TAG_QF_BV_HPP
#define HEADER_metaSMT_TAG_QF_BV_HPP

#include <variant>

#include "Logic.hpp"

namespace metaSMT {
  namespace logic {
    namespace QF_BV {
      /**
       * @brief tags for SMT metaSMT::logic::QF_BV
       **/
      namespace tag {

        // variable tag
        struct var_tag {
          unsigned id;
          unsigned width;
          typedef attr::ignore attribute;

          template <typename STREAM>
          friend STREAM &operator<<(STREAM &out, var_tag const &self) {
            return (out << "bv_var_tag[" << self.id << ',' << self.width << "]");
          }

          bool operator<(var_tag const &other) const { return id < other.id; }
        };

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

        // operation tags
        TAG(bit0, constant)
        TAG(bit1, constant)

        // unary
        TAG(bvnot, unary)
        TAG(bvneg, unary)

        // bitwise binary
        TAG(bvand, binary)
        TAG(bvnand, binary)
        TAG(bvor, binary)
        TAG(bvnor, binary)
        TAG(bvxor, binary)
        TAG(bvxnor, binary)

        TAG(bvcomp, binary)

        // bitvec arithmetic
        TAG(bvadd, binary)
        TAG(bvmul, binary)
        TAG(bvsub, binary)
        TAG(bvsdiv, binary)
        TAG(bvsrem, binary)
        TAG(bvudiv, binary)
        TAG(bvurem, binary)

        // bitvector constant creation
        TAG(bvuint, constant)
        TAG(bvsint, constant)
        TAG(bvbin, constant)
        TAG(bvhex, constant)

        // modifying bv length
        TAG(concat, binary)
        TAG(extract, unary)
        TAG(repeat, ignore)
        TAG(zero_extend, unary)
        TAG(sign_extend, unary)

        // bitvector shifting
        TAG(bvshl, binary)
        TAG(bvshr, binary)
        TAG(bvashr, binary)

        // comparison operators
        TAG(bvslt, binary)
        TAG(bvsgt, binary)
        TAG(bvsle, binary)
        TAG(bvsge, binary)
        TAG(bvult, binary)
        TAG(bvugt, binary)
        TAG(bvule, binary)
        TAG(bvuge, binary)

#undef PRINT
#undef TAG

        // tag variant QF_BV_Tag
        using QF_BV_Tag =
            std::variant<nil, bit0_tag, bit1_tag, bvnot_tag, bvneg_tag, bvand_tag, bvnand_tag, bvor_tag, bvnor_tag,
                         bvxor_tag, bvxnor_tag, bvcomp_tag, bvadd_tag, bvmul_tag, bvsub_tag, bvsrem_tag, bvsdiv_tag,
                         bvurem_tag, bvudiv_tag, bvuint_tag, bvsint_tag, bvbin_tag, bvhex_tag, bvslt_tag, bvsgt_tag,
                         bvsle_tag, bvsge_tag, bvult_tag, bvugt_tag, bvule_tag, bvuge_tag, concat_tag, extract_tag,
                         zero_extend_tag, sign_extend_tag, bvshl_tag, bvshr_tag, bvashr_tag, var_tag>;

      }  // namespace tag
      inline tag::var_tag new_bitvector(unsigned width = 1) {
        tag::var_tag tag;
        tag.id = impl::new_var_id();
        tag.width = width;
        return tag;
      }
    }  // namespace QF_BV
  }    // namespace logic
}  // namespace metaSMT
#endif  // HEADER_metaSMT_TAG_QF_BV_HPP
//  vim: ft=cpp:ts=2:sw=2:expandtab
