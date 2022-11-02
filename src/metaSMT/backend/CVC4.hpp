#pragma once

#include "../result_wrapper.hpp"
#include "../tags/Array.hpp"
#include "../tags/QF_BV.hpp"

#ifdef __clang__
#define _BACKWARD_BACKWARD_WARNING_H
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvariadic-macros"
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#ifdef __GNUC__
#if __GNUC__ >= 4 and __GNUC_MINOR__ > 4
// with this definitions gcc 4.4 creates executables with random segfaults
#define _BACKWARD_BACKWARD_WARNING_H
#pragma GCC push_options
#pragma GCC diagnostic ignored "-Wvariadic-macros"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#endif

#include <cvc4/cvc4.h>

#ifdef __GNUC__
#if __GNUC__ >= 4 and __GNUC_MINOR__ > 4
// this makes gcc 4.4 corrupt executables and cause random segfaults
#pragma GCC pop_options
#undef _BACKWARD_BACKWARD_WARNING_H
#endif
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#undef _BACKWARD_BACKWARD_WARNING_H
#endif

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
     * @class CVC4 CVC4.hpp metaSMT/backend/CVC4.hpp
     * @brief The CVC4 backend
     */
    class CVC4 {
     private:
      typedef std::tuple<uint64_t, unsigned> bvuint_tuple;
      typedef std::tuple<int64_t, unsigned> bvsint_tuple;

     public:
      typedef ::CVC4::Expr result_type;
      typedef std::list<::CVC4::Expr> Exprs;

      CVC4() : exprManager_(), engine_(&exprManager_), isPushed_(false) {
        engine_.setOption("incremental", true);
        engine_.setOption("produce-models", true);
        engine_.setLogic("QF_ABV");
      }

      ~CVC4() {}

      result_type operator()(arraytags::array_var_tag const &var, std::any const &) {
        if (var.id == 0) {
          throw std::runtime_error("uninitialized array used");
        }
        ::CVC4::Type elementType = exprManager_.mkBitVectorType(var.elem_width);
        ::CVC4::Type indexType = exprManager_.mkBitVectorType(var.index_width);
        ::CVC4::Type arrayType = exprManager_.mkArrayType(indexType, elementType);
        return exprManager_.mkVar(arrayType);
      }

      result_type operator()(arraytags::select_tag const &, result_type const &array, result_type const &index) {
        return exprManager_.mkExpr(::CVC4::kind::SELECT, array, index);
      }

      result_type operator()(arraytags::store_tag const &, result_type const &array, result_type const &index,
                             result_type const &value) {
        return exprManager_.mkExpr(::CVC4::kind::STORE, array, index, value);
      }

      void assertion(result_type e) { assertions_.push_back(e); }

      void assumption(result_type e) { assumptions_.push_back(e); }

      unsigned get_bv_width(result_type const &e) {
        ::CVC4::Type type = e.getType();
        return type.isBitVector() ? ((::CVC4::BitVectorType)type).getSize() : 0;
      }

      bool solve() {
        removeOldAssumptions();
        pushAssertions();
        pushAssumptions();

        return engine_.checkSat().isSat();
      }

      result_wrapper read_value(result_type var) {
        ::CVC4::Expr value = engine_.getValue(var);
        ::CVC4::Type type = value.getType();

        if (type.isBoolean()) {
          return value.getConst<bool>();

        } else if (type.isBitVector()) {
          ::CVC4::BitVector bvValue = value.getConst<::CVC4::BitVector>();
          return bvValue.toString();
        }
        return result_wrapper(false);
      }

      // predtags
      result_type operator()(predtags::var_tag const &, std::any) {
        return exprManager_.mkVar(exprManager_.booleanType());
      }

      result_type operator()(predtags::false_tag, std::any) { return exprManager_.mkConst(false); }

      result_type operator()(predtags::true_tag, std::any) { return exprManager_.mkConst(true); }

      result_type operator()(predtags::not_tag, result_type e) { return exprManager_.mkExpr(::CVC4::kind::NOT, e); }

      result_type operator()(predtags::ite_tag, result_type a, result_type b, result_type c) {
        return exprManager_.mkExpr(::CVC4::kind::ITE, a, b, c);
      }

      // bvtags
      result_type operator()(bvtags::var_tag const &var, std::any) {
        assert(var.width != 0);
        ::CVC4::Type bv_ty = exprManager_.mkBitVectorType(var.width);
        return exprManager_.mkVar(bv_ty);
      }

      result_type operator()(bvtags::bit0_tag, std::any) { return exprManager_.mkConst(::CVC4::BitVector(1u, 0u)); }

      result_type operator()(bvtags::bit1_tag, std::any) { return exprManager_.mkConst(::CVC4::BitVector(1u, 1u)); }

      result_type operator()(bvtags::bvuint_tag, std::any arg) {
        uint64_t value;
        unsigned width;
        std::tie(value, width) = std::any_cast<bvuint_tuple>(arg);
        return exprManager_.mkConst(::CVC4::BitVector(width, value));
      }

      result_type operator()(bvtags::bvsint_tag, std::any arg) {
        int64_t value;
        unsigned width;
        std::tie(value, width) = std::any_cast<bvsint_tuple>(arg);
        ::CVC4::BitVector bvValue(width, ::CVC4::Integer(value));
        return exprManager_.mkConst(bvValue);
      }

      result_type operator()(bvtags::bvbin_tag, std::any arg) {
        std::string val = std::any_cast<std::string>(arg);
        return exprManager_.mkConst(::CVC4::BitVector(val));
      }

      result_type operator()(bvtags::bvhex_tag, std::any arg) {
        std::string hex = std::any_cast<std::string>(arg);
        return exprManager_.mkConst(::CVC4::BitVector(hex, 16));
      }

      result_type operator()(bvtags::bvnot_tag, result_type e) {
        return exprManager_.mkExpr(::CVC4::kind::BITVECTOR_NOT, e);
      }

      result_type operator()(bvtags::bvneg_tag, result_type e) {
        return exprManager_.mkExpr(::CVC4::kind::BITVECTOR_NEG, e);
      }

      result_type operator()(bvtags::extract_tag const &, unsigned upper, unsigned lower, result_type e) {
        ::CVC4::BitVectorExtract bvOp(upper, lower);
        ::CVC4::Expr op = exprManager_.mkConst(bvOp);
        return exprManager_.mkExpr(op, e);
      }

      result_type operator()(bvtags::zero_extend_tag const &, unsigned width, result_type e) {
        ::CVC4::BitVectorZeroExtend bvOp(width);
        ::CVC4::Expr op = exprManager_.mkConst(bvOp);
        return exprManager_.mkExpr(op, e);
      }

      result_type operator()(bvtags::sign_extend_tag const &, unsigned width, result_type e) {
        ::CVC4::BitVectorSignExtend bvOp(width);
        ::CVC4::Expr op = exprManager_.mkConst(bvOp);
        return exprManager_.mkExpr(op, e);
      }

      result_type operator()(predtags::equal_tag const &, result_type a, result_type b) {
        /*#ifndef CVC4_WITHOUT_KIND_IFF
                if (a.getType().isBoolean() && b.getType().isBoolean()) {
                  return exprManager_.mkExpr(::CVC4::kind::IFF, a, b);
                } else {
                  return exprManager_.mkExpr(::CVC4::kind::EQUAL, a, b);
                }
        #else*/
        return exprManager_.mkExpr(::CVC4::kind::EQUAL, a, b);
      }

      result_type operator()(predtags::nequal_tag const &, result_type a, result_type b) {
        return exprManager_.mkExpr(::CVC4::kind::DISTINCT, a, b);
      }

      result_type operator()(predtags::distinct_tag const &, result_type a, result_type b) {
        return exprManager_.mkExpr(::CVC4::kind::DISTINCT, a, b);
      }

      ////////////////////////
      // Fallback operators //
      ////////////////////////

      template <typename TagT>
      result_type operator()(TagT, std::any) {
        assert(false);
        return exprManager_.mkConst(false);
      }

      template <::CVC4::kind::Kind_t KIND_>
      struct Op2 {
        static result_type exec(::CVC4::ExprManager &em, ::CVC4::Expr x, ::CVC4::Expr y) {
          return em.mkExpr(KIND_, x, y);
        }
      };

      template <::CVC4::kind::Kind_t KIND_>
      struct NotOp2 {
        static result_type exec(::CVC4::ExprManager &em, ::CVC4::Expr x, ::CVC4::Expr y) {
          return em.mkExpr(::CVC4::kind::NOT, em.mkExpr(KIND_, x, y));
        }
      };

      result_type operator()(predtags::and_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::AND>::exec(exprManager_, a, b);
      }
      result_type operator()(predtags::nand_tag, result_type a, result_type b) {
        return NotOp2<::CVC4::kind::AND>::exec(exprManager_, a, b);
      }
      result_type operator()(predtags::or_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::OR>::exec(exprManager_, a, b);
      }
      result_type operator()(predtags::nor_tag, result_type a, result_type b) {
        return NotOp2<::CVC4::kind::OR>::exec(exprManager_, a, b);
      }
      result_type operator()(predtags::xor_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::XOR>::exec(exprManager_, a, b);
      }
      result_type operator()(predtags::xnor_tag, result_type a, result_type b) {
        return NotOp2<::CVC4::kind::XOR>::exec(exprManager_, a, b);
      }
      result_type operator()(predtags::implies_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::IMPLIES>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvand_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_AND>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvnand_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_NAND>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvor_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_OR>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvnor_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_NOR>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvxor_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_XOR>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvxnor_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_XNOR>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvadd_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_PLUS>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvsub_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_SUB>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvmul_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_MULT>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvudiv_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_UDIV>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvurem_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_UREM>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvsdiv_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_SDIV>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvsrem_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_SREM>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvslt_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_SLT>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvsle_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_SLE>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvsgt_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_SGT>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvsge_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_SGE>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvult_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_ULT>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvule_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_ULE>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvugt_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_UGT>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvuge_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_UGE>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::concat_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_CONCAT>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvcomp_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_COMP>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvshl_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_SHL>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvshr_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_LSHR>::exec(exprManager_, a, b);
      }
      result_type operator()(bvtags::bvashr_tag, result_type a, result_type b) {
        return Op2<::CVC4::kind::BITVECTOR_ASHR>::exec(exprManager_, a, b);
      }

      template <typename TagT>
      result_type operator()(TagT, result_type a, result_type b) {
        assert(false && "unknown operator");
        return exprManager_.mkConst(false);
      }

      template <typename TagT>
      result_type operator()(TagT, result_type, result_type, result_type) {
        assert(false);
        return exprManager_.mkConst(false);
      }

      // pseudo command
      void command(CVC4 const &) {}

     private:
      void removeOldAssumptions() {
        if (isPushed_) {
          engine_.pop();
          isPushed_ = false;
        }
      }

      void pushAssumptions() {
        engine_.push();
        isPushed_ = true;

        applyAssertions(assumptions_);
        assumptions_.clear();
      }

      void pushAssertions() {
        applyAssertions(assertions_);
        assertions_.clear();
      }

      void applyAssertions(Exprs const &expressions) {
        for (Exprs::const_iterator it = expressions.begin(), ie = expressions.end(); it != ie; ++it) {
          engine_.assertFormula(*it);
        }
      }

     private:
      ::CVC4::ExprManager exprManager_;
      ::CVC4::SmtEngine engine_;
      Exprs assumptions_;
      Exprs assertions_;
      bool isPushed_;
    };  // class CVC4

  }  // namespace solver
}  // namespace metaSMT
