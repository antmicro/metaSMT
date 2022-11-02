#pragma once

#include "../result_wrapper.hpp"
#include "../tags/Array.hpp"
#include "../tags/QF_BV.hpp"

extern "C" {
#include <boolector/boolector.h>
}

#include <any>
#include <list>
#include <tuple>

namespace metaSMT {
  namespace solver {

    namespace predtags = ::metaSMT::logic::tag;
    namespace bvtags = ::metaSMT::logic::QF_BV::tag;
    namespace arraytags = ::metaSMT::logic::Array::tag;

    /**
     * @ingroup Backend
     * @class Boolector Boolector.hpp metaSMT/backend/Boolector.hpp
     * @brief The Boolector backend
     */
    class Boolector {
     private:
      typedef std::tuple<uint64_t, unsigned> bvuint_tuple;
      typedef std::tuple<int64_t, unsigned> bvsint_tuple;

     public:
      typedef BoolectorNode *result_type;

      Boolector() {
        _btor = boolector_new();
        boolector_set_opt(_btor, BTOR_OPT_MODEL_GEN, 1);
        boolector_set_opt(_btor, BTOR_OPT_INCREMENTAL, 1);
      }

      ~Boolector() {
        boolector_release_all(_btor);
        boolector_delete(_btor);
      }

      result_wrapper read_value(result_type var) {
        const char *value = boolector_bv_assignment(_btor, var);
        std::string s(value);
        boolector_free_bv_assignment(_btor, value);
        return result_wrapper(s);
      }

      result_type ptr(result_type expr) { return expr; }

     public:
      void assertion(result_type e) { boolector_assert(_btor, e); }

      void assumption(result_type e) { boolector_assume(_btor, e); }

      unsigned get_bv_width(result_type const &e) { return boolector_get_width(_btor, e); }

      bool solve() { return boolector_sat(_btor) == BOOLECTOR_SAT; }

      //#ifdef metaSMT_BOOLECTOR_2_NEW_API

#define _bv_sort(w) boolector_bitvec_sort(_btor, (w))
#define _array_sort(e, i) boolector_array_sort(_btor, _bv_sort(i), _bv_sort(e))
#define _bool_sort boolector_bool_sort(_btor)

      /*#else

      #define _bv_sort(w) w
      #define _array_sort(e, i) e, i
      #define _bool_sort 1

      #endif*/

      result_type operator()(predtags::var_tag const &, std::any) {
        return ptr(boolector_var(_btor, _bool_sort, NULL));
      }

      result_type operator()(predtags::false_tag, std::any) { return ptr(boolector_false(_btor)); }

      result_type operator()(predtags::true_tag, std::any) { return ptr(boolector_true(_btor)); }

      result_type operator()(predtags::not_tag, result_type a) { return ptr(boolector_not(_btor, a)); }

      result_type operator()(bvtags::var_tag const &var, std::any) {
        assert(var.width != 0);
        return ptr(boolector_var(_btor, _bv_sort(var.width), NULL));
      }

      result_type operator()(bvtags::bit0_tag, std::any) { return ptr(boolector_false(_btor)); }

      result_type operator()(bvtags::bit1_tag, std::any) { return ptr(boolector_true(_btor)); }

      result_type operator()(bvtags::bvuint_tag, std::any arg) {
        uint64_t value;
        unsigned width;
        std::tie(value, width) = std::any_cast<bvuint_tuple>(arg);

        if (value > std::numeric_limits<unsigned>::max()) {
          std::string val(width, '0');

          std::string::reverse_iterator sit = val.rbegin();

          for (unsigned i = 0; i < width; i++, ++sit) {
            *sit = (value & 1ul) ? '1' : '0';
            value >>= 1;
          }
          return ptr(boolector_const(_btor, val.c_str()));
        } else {
          return ptr(boolector_unsigned_int(_btor, value, _bv_sort(width)));
        }
      }

      result_type operator()(bvtags::bvsint_tag, std::any arg) {
        int64_t value;
        unsigned width;
        std::tie(value, width) = std::any_cast<bvsint_tuple>(arg);

        if (value > std::numeric_limits<int>::max() || value < std::numeric_limits<int>::min()) {
          std::string val(width, '0');

          std::string::reverse_iterator sit = val.rbegin();

          for (unsigned i = 0; i < width; i++, ++sit) {
            *sit = (value & 1l) ? '1' : '0';
            value >>= 1;
          }
          return ptr(boolector_const(_btor, val.c_str()));
        } else {
          return ptr(boolector_int(_btor, value, _bv_sort(width)));
        }
      }

      result_type operator()(bvtags::bvbin_tag, std::any arg) {
        std::string val = std::any_cast<std::string>(arg);
        return ptr(boolector_const(_btor, val.c_str()));
      }

      result_type operator()(bvtags::bvhex_tag, std::any arg) {
        std::string hex = std::any_cast<std::string>(arg);
        std::string bin(hex.size() * 4, '\0');

        for (unsigned i = 0; i < hex.size(); ++i) {
          switch (tolower(hex[i])) {
            case '0':
              bin.replace(4 * i, 4, "0000");
              break;
            case '1':
              bin.replace(4 * i, 4, "0001");
              break;
            case '2':
              bin.replace(4 * i, 4, "0010");
              break;
            case '3':
              bin.replace(4 * i, 4, "0011");
              break;
            case '4':
              bin.replace(4 * i, 4, "0100");
              break;
            case '5':
              bin.replace(4 * i, 4, "0101");
              break;
            case '6':
              bin.replace(4 * i, 4, "0110");
              break;
            case '7':
              bin.replace(4 * i, 4, "0111");
              break;
            case '8':
              bin.replace(4 * i, 4, "1000");
              break;
            case '9':
              bin.replace(4 * i, 4, "1001");
              break;
            case 'a':
              bin.replace(4 * i, 4, "1010");
              break;
            case 'b':
              bin.replace(4 * i, 4, "1011");
              break;
            case 'c':
              bin.replace(4 * i, 4, "1100");
              break;
            case 'd':
              bin.replace(4 * i, 4, "1101");
              break;
            case 'e':
              bin.replace(4 * i, 4, "1110");
              break;
            case 'f':
              bin.replace(4 * i, 4, "1111");
              break;
          }
        }
        // std::cout << bin << std::endl;
        return ptr(boolector_const(_btor, bin.c_str()));
      }

      result_type operator()(bvtags::bvnot_tag, result_type a) { return ptr(boolector_not(_btor, a)); }

      result_type operator()(bvtags::bvneg_tag, result_type a) { return ptr(boolector_neg(_btor, a)); }

      result_type operator()(bvtags::extract_tag const &, unsigned upper, unsigned lower, result_type e) {
        return ptr(boolector_slice(_btor, e, upper, lower));
      }

      result_type operator()(bvtags::zero_extend_tag const &, unsigned width, result_type e) {
        return ptr(boolector_uext(_btor, e, width));
      }

      result_type operator()(bvtags::sign_extend_tag const &, unsigned width, result_type e) {
        return ptr(boolector_sext(_btor, e, width));
      }

      result_type operator()(arraytags::array_var_tag const &var, std::any) {
        return ptr(boolector_array(_btor, _array_sort(var.elem_width, var.index_width), NULL));
      }

      result_type operator()(arraytags::select_tag const &, result_type array, result_type index) {
        return ptr(boolector_read(_btor, array, index));
      }

      result_type operator()(arraytags::store_tag const &, result_type array, result_type index, result_type value) {
        return ptr(boolector_write(_btor, array, index, value));
      }

      result_type operator()(predtags::ite_tag, result_type a, result_type b, result_type c) {
        return ptr(boolector_cond(_btor, a, b, c));
      }

      ////////////////////////
      // Fallback operators //
      ////////////////////////

      template <typename TagT>
      result_type operator()(TagT, std::any) {
        assert(false && "unknown operator");
        return ptr(boolector_false(_btor));
      }

      template <typename TagT>
      result_type operator()(TagT, result_type) {
        assert(false && "unknown operator");
        return ptr(boolector_false(_btor));
      }

      template <result_type (*FN)(Btor *, result_type, result_type)>
      struct Btor_F2 {
        static result_type exec(Btor *b, result_type x, result_type y) { return (*FN)(b, x, y); }
      };

      result_type operator()(predtags::and_tag const &, std::vector<result_type> const &rs) {
        // resolve left associativity
        result_type r = this->operator()(predtags::and_tag(), rs[0], rs[1]);
        for (std::size_t u = 2; u < rs.size(); ++u) {
          r = this->operator()(predtags::and_tag(), r, rs[u]);
        }
        return r;
      }

      result_type operator()(predtags::or_tag const &, std::vector<result_type> const &rs) {
        // resolve left associativity
        result_type r = this->operator()(predtags::or_tag(), rs[0], rs[1]);
        for (std::size_t u = 2; u < rs.size(); ++u) {
          r = this->operator()(predtags::or_tag(), r, rs[u]);
        }
        return r;
      }

      result_type operator()(predtags::equal_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_eq>::exec(_btor, a, b));
      }
      result_type operator()(predtags::nequal_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_ne>::exec(_btor, a, b));
      }
      result_type operator()(predtags::distinct_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_ne>::exec(_btor, a, b));
      }
      result_type operator()(predtags::and_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_and>::exec(_btor, a, b));
      }
      result_type operator()(predtags::nand_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_nand>::exec(_btor, a, b));
      }
      result_type operator()(predtags::or_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_or>::exec(_btor, a, b));
      }
      result_type operator()(predtags::nor_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_nor>::exec(_btor, a, b));
      }
      result_type operator()(predtags::xor_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_xor>::exec(_btor, a, b));
      }
      result_type operator()(predtags::xnor_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_xnor>::exec(_btor, a, b));
      }
      result_type operator()(predtags::implies_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_implies>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvand_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_and>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvnand_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_nand>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvor_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_or>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvnor_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_nor>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvxor_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_xor>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvxnor_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_xnor>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvadd_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_add>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvsub_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_sub>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvmul_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_mul>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvudiv_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_udiv>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvurem_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_urem>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvsdiv_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_sdiv>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvsrem_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_srem>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvslt_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_slt>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvsle_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_slte>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvsgt_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_sgt>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvsge_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_sgte>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvult_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_ult>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvule_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_ulte>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvugt_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_ugt>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvuge_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_ugte>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::concat_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_concat>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvshl_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_sll>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvshr_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_srl>::exec(_btor, a, b));
      }
      result_type operator()(bvtags::bvashr_tag, result_type a, result_type b) {
        return ptr(Btor_F2<&boolector_sra>::exec(_btor, a, b));
      }

      template <typename TagT>
      result_type operator()(TagT tag, result_type a, result_type b) {
        assert(false && "unknown operator");
        return ptr(boolector_false(_btor));
      }

      template <typename TagT>
      result_type operator()(TagT, result_type, result_type, result_type) {
        assert(false && "unknown operator");
        return ptr(boolector_false(_btor));
      }

      /* pseudo command */
      void command(Boolector const &) {}

     protected:
      Btor *_btor;
    };

    /**@}*/

  }  // namespace solver
}  // namespace metaSMT

//  vim: ft=cpp:ts=2:sw=2:expandtab
