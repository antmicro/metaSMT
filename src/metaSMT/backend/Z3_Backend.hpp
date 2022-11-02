#pragma once
#include <z3++.h>

#include <any>
#include <boost/multiprecision/cpp_int.hpp>
#include <limits>
#include <tuple>

#include "../Features.hpp"
#include "../result_wrapper.hpp"
#include "../tags/Array.hpp"
#include "../tags/QF_BV.hpp"
#include "../tags/QF_UF.hpp"

namespace metaSMT {

  // forward declare stack api
  struct stack_pop;
  struct stack_push;
  namespace features {
    struct stack_api;
  }  // namespace features

  namespace solver {
    namespace predtags = ::metaSMT::logic::tag;
    namespace bvtags = ::metaSMT::logic::QF_BV::tag;
    namespace arraytags = ::metaSMT::logic::Array::tag;
    namespace uftags = ::metaSMT::logic::QF_UF::tag;

    namespace detail {
      struct dummy {
        dummy() {}
      };  // dummy

      struct equals {
        equals() {}

        template <typename T1, typename T2>
        bool operator()(T1 const &, T2 const &) const {
          return false;
        }

        bool operator()(dummy const &, dummy const &) const { return true; }

        bool operator()(z3::func_decl const &lhs, z3::func_decl const &rhs) const { return eq(lhs, rhs); }

        bool operator()(z3::expr const &lhs, z3::expr const &rhs) const { return eq(lhs, rhs); }
      };  // equals

      struct domain_sort_visitor {
        domain_sort_visitor(Z3_context &ctx) : ctx(ctx) {}

        Z3_sort operator()(type::Boolean const &) const { return Z3_mk_bool_sort(ctx); }

        Z3_sort operator()(type::BitVector const &arg) const { return Z3_mk_bv_sort(ctx, arg.width); }

        Z3_sort operator()(type::Array const &arg) const {
          Z3_sort index_type = Z3_mk_bv_sort(ctx, arg.index_width);
          Z3_sort elem_type = Z3_mk_bv_sort(ctx, arg.elem_width);
          return Z3_mk_array_sort(ctx, index_type, elem_type);
        }

        Z3_context &ctx;
      };  // domain_sort_visitor
    }     // namespace detail

    class Z3_Backend {
     private:
      typedef std::tuple<uint64_t, unsigned> bvuint_tuple;
      typedef std::tuple<int64_t, unsigned> bvsint_tuple;

     public:
      struct result_type {
        // first type in variant has to be default constructable
        std::variant<detail::dummy, z3::func_decl, z3::expr> internal;

        result_type() {}

        result_type(z3::expr a) : internal(a) {}

        result_type(z3::func_decl a) : internal(a) {}

        inline operator z3::expr() const { return std::get<z3::expr>(internal); }

        inline operator z3::func_decl() const { return std::get<z3::func_decl>(internal); }

        inline bool operator==(result_type const &r) const {
          return std::visit(detail::equals(), internal, r.internal);
        }

        inline bool operator!=(result_type const &r) const { return !operator==(r); }
#if 0
        inline bool operator <(result_type const &r) const {
          assert( false && "Yet not implemented" );
          return false;
        }
#endif
      };  // result_type

      // typedef z3::ast result_type;

      Z3_Backend() : solver_(ctx_, "QF_AUFBV"), assumption_((*this)(predtags::true_tag(), std::any())) {}

      ~Z3_Backend() {}

      result_wrapper read_value(result_type const &var) {
        z3::model model = solver_.get_model();
        z3::expr r = model.eval(z3::expr(var), /* completion = */ true);

        // predicate
        if (r.is_bool()) {
          Z3_lbool b = Z3_get_bool_value(ctx_, r);
          if (b == Z3_L_FALSE) return result_wrapper(false);
          if (b == Z3_L_TRUE) return result_wrapper(true);
          return result_wrapper();
        }

        assert(r.is_bv());
        uint64_t val = 0;
        if (Z3_get_numeral_uint64(ctx_, r, &val) && val <= std::numeric_limits<uint64_t>::max())
          return result_wrapper(val, r.get_sort().bv_size());

        std::string str = Z3_ast_to_string(ctx_, r);
        assert(str.find("#b") == 0);
        return result_wrapper(str.substr(2));
      }

      void assertion(result_type const &e) { solver_.add(static_cast<z3::expr const &>(e)); }

      void assumption(result_type const &e) { assumption_ = (*this)(predtags::and_tag(), assumption_, e); }

      unsigned get_bv_width(result_type const &e) {
        z3::expr r = z3::expr(e);
        return r.is_bv() ? r.get_sort().bv_size() : 0;
      }

      bool solve() {
        solver_.push();
        assertion(assumption_);
        z3::check_result result = solver_.check(ctx_);
        // std::cerr << result << '\n';
        solver_.pop();
        assumption_ = (*this)(predtags::true_tag(), std::any());
        return (result == z3::sat);
      }

      result_type operator()(predtags::var_tag const &var, std::any) {
        char buf[64];
        sprintf(buf, "var_%u", var.id);
        return ctx_.bool_const(buf);
      }

      result_type operator()(bvtags::var_tag const &var, std::any) {
        char buf[64];
        sprintf(buf, "var_%u", var.id);
        return ctx_.bv_const(buf, var.width);
      }

      result_type operator()(arraytags::array_var_tag const &var, std::any const &) {
        if (var.id == 0) {
          throw std::runtime_error("uninitialized array used");
        }

        z3::sort index_sort = ctx_.bv_sort(var.index_width);
        z3::sort elem_sort = ctx_.bv_sort(var.elem_width);
        z3::sort ty = ctx_.array_sort(index_sort, elem_sort);

        char buf[64];
        sprintf(buf, "var_%u", var.id);

        Z3_symbol s = Z3_mk_string_symbol(ctx_, buf);
        return z3::to_expr(ctx_, Z3_mk_const(ctx_, s, ty));
      }

      result_type operator()(uftags::function_var_tag const &var, std::any) {
        unsigned const num_args = var.args.size();

        // construct the name of the uninterpreted_function
        char buf[64];
        sprintf(buf, "function_var_%u", var.id);

        Z3_symbol sym = ctx_.str_symbol(buf);

        // construct result sort
        Z3_context ctx = ctx_;
        Z3_sort result_sort = std::visit(detail::domain_sort_visitor(ctx), var.result_type);

        // construct argument sorts
        Z3_sort *domain_sort = new Z3_sort[num_args];
        for (unsigned u = 0; u < num_args; ++u) {
          domain_sort[u] = std::visit(detail::domain_sort_visitor(ctx), var.args[u]);
        }

        return z3::to_func_decl(ctx_, Z3_mk_func_decl(ctx_, sym, num_args, domain_sort, result_sort));
      }

      result_type operator()(predtags::false_tag const &, std::any) { return ctx_.bool_val(false); }

      result_type operator()(predtags::true_tag const &, std::any) { return ctx_.bool_val(true); }

      /* TODO: is this needed?
      result_type operator()(predtags::not_tag const &, result_type const &a) {
        return !static_cast<z3::expr const &>(a);
      }*/

      result_type operator()(predtags::nand_tag const &, result_type const &a, result_type const &b) {
        return (*this)(predtags::not_tag(), (*this)(predtags::and_tag(), z3::expr(a), z3::expr(b)));
      }

      result_type operator()(predtags::nor_tag const &, result_type const &a, result_type const &b) {
        return (*this)(predtags::not_tag(), (*this)(predtags::or_tag(), z3::expr(a), z3::expr(b)));
      }

      result_type operator()(predtags::xnor_tag const &, result_type const &a, result_type const &b) {
        return (*this)(predtags::not_tag(), (*this)(predtags::xor_tag(), z3::expr(a), z3::expr(b)));
      }

      result_type operator()(predtags::ite_tag, result_type const &a, result_type const &b, result_type const &c) {
        return z3::to_expr(ctx_, Z3_mk_ite(ctx_, z3::expr(a), z3::expr(b), z3::expr(c)));
      }

      result_type operator()(bvtags::bit0_tag, std::any) { return ctx_.bv_val(0, 1); }

      result_type operator()(bvtags::bit1_tag, std::any) { return ctx_.bv_val(1, 1); }

      result_type operator()(bvtags::bvuint_tag const &, std::any const &arg) {
        uint64_t value;
        unsigned width;
        std::tie(value, width) = std::any_cast<bvuint_tuple>(arg);
        Z3_sort ty = Z3_mk_bv_sort(ctx_, width);
        return z3::to_expr(ctx_, Z3_mk_unsigned_int64(ctx_, value, ty));
      }

      result_type operator()(bvtags::bvsint_tag const &, std::any const &arg) {
        int64_t value;
        unsigned width;
        std::tie(value, width) = std::any_cast<bvsint_tuple>(arg);
        Z3_sort ty = Z3_mk_bv_sort(ctx_, width);
        return z3::to_expr(ctx_, Z3_mk_int64(ctx_, value, ty));
      }

      result_type operator()(bvtags::bvbin_tag, std::any arg) {
        std::string s = std::any_cast<std::string>(arg);
        size_t bv_len = s.size();

        // FIXME: Remove this use of boost
        using boost::multiprecision::cpp_int;
        cpp_int value;
        for (unsigned i = 0; i < bv_len; i++)
          if (s[i] == '1') bit_set(value, bv_len - i - 1);

        result_type r = ctx_.bv_val(value.str().c_str(), bv_len);
        return r;
      }

      // XXX will be removed in a later revision
      result_type operator()(bvtags::bvhex_tag, std::any arg) {
        std::string const hex = std::any_cast<std::string>(arg);
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
        return (*this)(bvtags::bvbin_tag(), std::any(bin));
      }

      result_type operator()(bvtags::bvcomp_tag, result_type const &a, result_type const &b) {
        result_type bit1 = (*this)(bvtags::bit1_tag(), std::any());
        result_type bit0 = (*this)(bvtags::bit0_tag(), std::any());
        return z3::to_expr(ctx_,
                           Z3_mk_ite(ctx_, Z3_mk_eq(ctx_, z3::expr(a), z3::expr(b)), z3::expr(bit1), z3::expr(bit0)));
      }

      result_type operator()(bvtags::extract_tag const &, unsigned upper, unsigned lower, result_type const &e) {
        return z3::to_expr(ctx_, Z3_mk_extract(ctx_, upper, lower, z3::expr(e)));
      }

      result_type operator()(bvtags::zero_extend_tag const &, unsigned width, result_type e) {
        return z3::to_expr(ctx_, Z3_mk_zero_ext(ctx_, width, z3::expr(e)));
      }

      result_type operator()(bvtags::sign_extend_tag const &, unsigned width, result_type e) {
        return z3::to_expr(ctx_, Z3_mk_sign_ext(ctx_, width, z3::expr(e)));
      }

      result_type operator()(arraytags::select_tag const &, result_type const &array, result_type const &index) {
        return z3::select(z3::expr(array), z3::expr(index));
      }

      result_type operator()(arraytags::store_tag const &, result_type const &array, result_type const &index,
                             result_type const &value) {
        return z3::store(z3::expr(array), z3::expr(index), z3::expr(value));
      }

      result_type operator()(predtags::and_tag const &, std::vector<result_type> const &vs) {
        std::size_t const num_elm = vs.size();
        Z3_ast *args = new Z3_ast[num_elm];
        for (unsigned u = 0; u < num_elm; ++u) args[u] = z3::expr(vs[u]);
        return z3::to_expr(ctx_, Z3_mk_and(ctx_, num_elm, args));
      }

      result_type operator()(predtags::or_tag const &, std::vector<result_type> const &vs) {
        std::size_t const num_elm = vs.size();
        Z3_ast *args = new Z3_ast[num_elm];
        for (unsigned u = 0; u < num_elm; ++u) args[u] = z3::expr(vs[u]);
        return z3::to_expr(ctx_, Z3_mk_or(ctx_, num_elm, args));
      }

      template <Z3_ast (*FN)(Z3_context, Z3_ast)>
      struct Z3_F1 {
        static Z3_ast exec(Z3_context c, Z3_ast x) { return (*FN)(c, x); }
      };  // Z3_F1

      result_type operator()(predtags::not_tag, result_type a) {
        Z3_ast r = Z3_F1<&Z3_mk_not>::exec(ctx_, z3::expr(a));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvneg_tag, result_type a) {
        Z3_ast r = Z3_F1<&Z3_mk_bvneg>::exec(ctx_, z3::expr(a));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvnot_tag, result_type a) {
        Z3_ast r = Z3_F1<&Z3_mk_bvnot>::exec(ctx_, z3::expr(a));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }

      template <typename TagT>
      result_type operator()(TagT tag, result_type a) {
        std::cout << "unknown operator: " << tag << std::endl;
        assert(false && "unknown operator");
        return (*this)(predtags::false_tag(), std::any());
      }

      template <Z3_ast (*FN)(Z3_context, Z3_ast, Z3_ast)>
      struct Z3_F2 {
        static Z3_ast exec(Z3_context c, Z3_ast x, Z3_ast y) { return (*FN)(c, x, y); }
      };  // Z3_F2

      template <Z3_ast (*FN)(Z3_context, unsigned, Z3_ast const[])>
      struct Z3_F2_MULTI_ARG {
        static Z3_ast exec(Z3_context c, Z3_ast x, Z3_ast y) {
          Z3_ast args[2] = {x, y};
          return (*FN)(c, 2, args);
        }
      };  // Z3_F2_MULTI_ARG

      result_type operator()(predtags::equal_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_eq>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(predtags::nequal_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2_MULTI_ARG<&Z3_mk_distinct>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(predtags::distinct_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2_MULTI_ARG<&Z3_mk_distinct>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(predtags::and_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2_MULTI_ARG<&Z3_mk_and>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(predtags::or_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2_MULTI_ARG<&Z3_mk_or>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(predtags::xor_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_xor>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(predtags::implies_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_implies>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvand_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvand>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvnand_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvnand>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvor_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvor>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvnor_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvnor>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvxor_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvxor>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvxnor_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvxnor>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvadd_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvadd>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvsub_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvsub>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvmul_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvmul>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvudiv_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvudiv>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvurem_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvurem>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvsdiv_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvsdiv>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvsrem_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvsrem>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvslt_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvslt>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvsle_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvsle>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvsgt_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvsgt>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvsge_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvsge>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvult_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvult>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvule_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvule>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvugt_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvugt>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvuge_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvuge>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::concat_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_concat>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvshl_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvshl>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvshr_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvlshr>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }
      result_type operator()(bvtags::bvashr_tag, result_type a, result_type b) {
        Z3_ast r = Z3_F2<&Z3_mk_bvashr>::exec(ctx_, z3::expr(a), z3::expr(b));
        z3::expr(a).check_error();
        return z3::expr(z3::expr(a).ctx(), r);
      }

      template <typename TagT>
      result_type operator()(TagT tag, result_type a, result_type b) {
        assert(false && "unknown operator");
        return (*this)(predtags::false_tag(), std::any());
      }

      ////////////////////////
      // Fallback operators //
      ////////////////////////
      /*template <typename Tag, typename T>
      result_type operator()(Tag const &, T const &) {
        assert(false);
        throw std::exception();
      }

      template <typename Tag, typename T1, typename T2>
      result_type operator()(Tag const &, T1 const &, T2 const &) {
        assert(false);
        throw std::exception();
      }

      template <typename Tag, typename T1, typename T2, typename T3>
      result_type operator()(Tag const &, T1 const &, T2 const &, T3 const &) {
        assert(false);
        throw std::exception();
      }*/

      void command(stack_push const &, unsigned howmany) {
        while (howmany > 0) {
          solver_.push();
          --howmany;
        }
      }

      void command(stack_pop const &, unsigned howmany) { solver_.pop(howmany); }

     private:
      z3::context ctx_;
      z3::solver solver_;
      result_type assumption_;
    };  // Z3_Backend
  }     // namespace solver

  namespace features {
    template <>
    struct supports<solver::Z3_Backend, features::stack_api> : std::true_type {};
  }  // namespace features
}  // namespace metaSMT
