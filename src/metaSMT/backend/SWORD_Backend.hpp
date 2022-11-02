#pragma once

#include <libsword.h>

#include <cstdio>
#include <type_traits>

#include "../result_wrapper.hpp"
#include "../tags/QF_BV.hpp"

namespace metaSMT {
  namespace solver {

    namespace bvtags = ::metaSMT::logic::QF_BV::tag;
    namespace predtags = ::metaSMT::logic::tag;

    /**
     * @ingroup Backend
     * @class SWORD_Backend SWORD_Backend.hpp metaSMT/backend/SWORD_Backend.hpp
     * @brief SWORD backend
     */
    class SWORD_Backend {
     private:
      typedef std::tuple<uint64_t, unsigned> bvuint_tuple;
      typedef std::tuple<int64_t, unsigned> bvsint_tuple;

     private:
      template <SWORD::OPCODE OC>
      struct SWORD_Op : public std::integral_constant<SWORD::OPCODE, OC> {};

     public:
      typedef SWORD::PSignal result_type;

      SWORD_Backend() {
        //_sword.recordTo("/tmp/sword.log");
      }

      /********************
       * predicate logic
       *******************/

      result_type operator()(predtags::var_tag const &var, std::any) {
        char buf[256];
        sprintf(buf, "pvar_%d", var.id);
        // printf("predicate variable created %s\n", buf);
        return _sword.addVariable(1, buf);
      }

      result_type operator()(predtags::false_tag, std::any) {
        // printf("false\n");
        return _sword.addConstant(1, 0);
      }

      result_type operator()(predtags::true_tag, std::any) {
        // printf("true\n");
        return _sword.addConstant(1, 1);
      }

      // QF_BV tags

      result_type operator()(bvtags::var_tag const &var, std::any) {
        char buf[256];
        sprintf(buf, "var_%d", var.id);
        // printf("variable created %s\n", buf);
        return _sword.addVariable(var.width, buf);
      }

      result_type operator()(bvtags::bit0_tag, std::any) {
        // printf("bit0\n");
        return _sword.addConstant(1, 0);
      }

      result_type operator()(bvtags::bit1_tag, std::any) {
        // printf("bit1\n");
        return _sword.addConstant(1, 1);
      }

      result_type operator()(bvtags::bvbin_tag, std::any arg) {
        // printf("bvuint\n");
        return _sword.addBinConstant(std::any_cast<std::string>(arg));
      }

      result_type operator()(bvtags::bvhex_tag, std::any arg) {
        // printf("bvuint\n");
        return _sword.addHexConstant(std::any_cast<std::string>(arg));
      }

      result_type operator()(bvtags::bvuint_tag, std::any arg) {
        uint64_t value;
        unsigned width;
        std::tie(value, width) = std::any_cast<bvuint_tuple>(arg);

        if (value > std::numeric_limits<unsigned long>::max()) {
          std::string val(width, '0');
          std::string::reverse_iterator it = val.rbegin();
          for (unsigned u = 0; u < width; ++u, ++it) {
            *it = (value & 1ul) ? '1' : '0';
            value >>= 1;
          }
          return _sword.addBinConstant(val);
        }
        return _sword.addConstant(width, static_cast<unsigned long>(value));
      }

      result_type operator()(bvtags::bvsint_tag, std::any arg) {
        int64_t value;
        unsigned width;
        std::tie(value, width) = std::any_cast<bvsint_tuple>(arg);

        if (value > std::numeric_limits<int>::max() || value < std::numeric_limits<int>::min() ||
            (width > 8 * sizeof(int) && value < 0)) {
          std::string val(width, '0');
          std::string::reverse_iterator it = val.rbegin();
          for (unsigned u = 0; u < width; ++u, ++it) {
            *it = (value & 1l) ? '1' : '0';
            value >>= 1;
          }
          return _sword.addBinConstant(val);
        }
        return _sword.addConstant(width, static_cast<unsigned long>(value));
      }

      result_type operator()(bvtags::extract_tag const &, unsigned upper, unsigned lower, result_type e) {
        return _sword.addExtract(e, upper, lower);
      }

      result_type operator()(bvtags::zero_extend_tag const &, unsigned width, result_type e) {
        return _sword.addZeroExtend(e, width);
      }

      result_type operator()(bvtags::sign_extend_tag const &, unsigned width, result_type e) {
        return _sword.addSignExtend(e, width);
      }

      template <typename TagT>
      result_type operator()(TagT tag, std::any) {
        // std::cout << tag << std::endl;
        return NULL;
      }

      template <typename TagT>
      result_type operator()(TagT tag, result_type a) {
        return (*this)(tag, a, NULL, NULL);
      }

      template <typename TagT>
      result_type operator()(TagT tag, result_type a, result_type b) {
        return (*this)(tag, a, b, NULL);
      }

      result_type operator()(metaSMT::nil, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::UNKNOWN>::value, a, b, c);
      }
      result_type operator()(predtags::not_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::NOT>::value, a, b, c);
      }
      result_type operator()(predtags::equal_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::EQUAL>::value, a, b, c);
      }
      result_type operator()(predtags::nequal_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::NEQUAL>::value, a, b, c);
      }
      result_type operator()(predtags::distinct_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::NEQUAL>::value, a, b, c);
      }
      result_type operator()(predtags::and_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::AND>::value, a, b, c);
      }
      result_type operator()(predtags::nand_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::NAND>::value, a, b, c);
      }
      result_type operator()(predtags::or_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::OR>::value, a, b, c);
      }
      result_type operator()(predtags::nor_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::NOR>::value, a, b, c);
      }
      result_type operator()(predtags::xor_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::XOR>::value, a, b, c);
      }
      result_type operator()(predtags::xnor_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::XNOR>::value, a, b, c);
      }
      result_type operator()(predtags::implies_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::IMPLIES>::value, a, b, c);
      }
      result_type operator()(bvtags::bvnot_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::NOT>::value, a, b, c);
      }
      result_type operator()(bvtags::bvneg_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::NEG>::value, a, b, c);
      }
      result_type operator()(bvtags::bvand_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::AND>::value, a, b, c);
      }
      result_type operator()(bvtags::bvnand_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::NAND>::value, a, b, c);
      }
      result_type operator()(bvtags::bvor_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::OR>::value, a, b, c);
      }
      result_type operator()(bvtags::bvnor_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::NOR>::value, a, b, c);
      }
      result_type operator()(bvtags::bvxor_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::XOR>::value, a, b, c);
      }
      result_type operator()(bvtags::bvxnor_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::XNOR>::value, a, b, c);
      }
      result_type operator()(bvtags::bvadd_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::ADD>::value, a, b, c);
      }
      result_type operator()(bvtags::bvsub_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::SUB>::value, a, b, c);
      }
      result_type operator()(bvtags::bvmul_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::MUL>::value, a, b, c);
      }
      result_type operator()(bvtags::bvudiv_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::UDIV>::value, a, b, c);
      }
      result_type operator()(bvtags::bvurem_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::UREM>::value, a, b, c);
      }
      result_type operator()(bvtags::bvsdiv_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::SDIV>::value, a, b, c);
      }
      result_type operator()(bvtags::bvsrem_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::SREM>::value, a, b, c);
      }
      result_type operator()(bvtags::bvslt_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::SLT>::value, a, b, c);
      }
      result_type operator()(bvtags::bvsle_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::SLE>::value, a, b, c);
      }
      result_type operator()(bvtags::bvsgt_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::SGT>::value, a, b, c);
      }
      result_type operator()(bvtags::bvsge_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::SGE>::value, a, b, c);
      }
      result_type operator()(bvtags::bvult_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::ULT>::value, a, b, c);
      }
      result_type operator()(bvtags::bvule_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::ULE>::value, a, b, c);
      }
      result_type operator()(bvtags::bvugt_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::UGT>::value, a, b, c);
      }
      result_type operator()(bvtags::bvuge_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::UGE>::value, a, b, c);
      }
      result_type operator()(bvtags::concat_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::CONCAT>::value, a, b, c);
      }
      result_type operator()(bvtags::bvcomp_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::EQUAL>::value, a, b, c);
      }
      result_type operator()(bvtags::bvshl_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::LSHL>::value, a, b, c);
      }
      result_type operator()(bvtags::bvshr_tag, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::LSHR>::value, a, b, c);
      }
      result_type operator()(bvtags::bvashr, result_type a, result_type b, result_type c) {
        return _sword.addOperator(SWORD_Op<SWORD::ASHR>::value, a, b, c);
      }

      void assertion(result_type e) { _sword.addAssertion(e); }

      void assumption(result_type e) { _sword.addAssumption(e); }

      unsigned get_bv_width(result_type const &e) {
        return 0;  // unsupported
      }

      bool solve() { return _sword.solve(); }

      result_wrapper read_value(result_type var) {
        const std::vector<int> val = _sword.getVariableAssignment(var);
        std::vector<tribool> ret(val.size());
        std::vector<tribool>::iterator it = ret.begin();
        for (unsigned i = 0; i < val.size(); ++i, ++it) {
          switch (val[i]) {
            case 0:
              *it = false;
              break;
            case 1:
              *it = true;
              break;
            default:
              *it = indeterminate;
              // printf("dc\n");
          }
        }
        return result_wrapper(ret);
      }

      /* pseudo command */
      void command(SWORD_Backend const &) {}

     private:
      SWORD::sword _sword;
    };

  }  // namespace solver
}  // namespace metaSMT

//  vim: ft=cpp:ts=2:sw=2:expandtab
