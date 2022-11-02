#pragma once

#include <any>
#include <cassert>
#include <unordered_map>

#include "API/Assertion.hpp"
#include "API/Assumption.hpp"
#include "API/Evaluator.hpp"
#include "API/Options.hpp"
#include "Features.hpp"
#include "result_wrapper.hpp"
#include "support/Options.hpp"
#include "tags/Array.hpp"
#include "tags/Logic.hpp"
#include "tags/QF_BV.hpp"
#include "tags/QF_UF.hpp"

namespace metaSMT {
  /**
   * @brief direct Solver integration
   *
   *  DirectSolver_Context takes a SolverType and directly feeds all commands
   *  to it. Variable expressions are cached and only evaluated once.
   **/
  template <typename SolverContext>
  struct DirectSolver_Context : public SolverContext {
    DirectSolver_Context() = default;
    // DirectSolver_Context() {
    //   typedef typename mpl::if_<
    //       /* if   = */ typename features::supports<SolverContext, setup_option_map_cmd>::type,
    //       /* then = */ option::SetupOptionMapCommand, /* else = */ option::NOPCommand>::type Command;
    //   Command::template action(static_cast<SolverContext &>(*this), opt);
    // }

    DirectSolver_Context(Options const &opt) : opt(opt) {}

    /// The returned expression type is the result_type of the SolverContext
    typedef typename SolverContext::result_type result_type;

    result_type operator()(bool value) {
      if (value)
        return (*this)(logic::tag::true_tag{}, std::any{});
      else
        return (*this)(logic::tag::false_tag{}, std::any{});
    }

    // FIXME:
    int operator()(int value) { return value; }

    // special handling of bvuint_tag
    result_type operator()(logic::QF_BV::tag::bvuint_tag const &tag, uint64_t value, unsigned bw) {
      return SolverContext::operator()(tag, std::any(std::make_tuple(value, bw)));
    }

    // special handling of bvsint_tag
    result_type operator()(logic::QF_BV::tag::bvsint_tag const &tag, uint64_t value, unsigned bw) {
      return SolverContext::operator()(tag, std::any(std::make_tuple((int64_t) /*FIXME*/ value, bw)));
    }

    // special handling of bvbin_tag
    result_type operator()(logic::QF_BV::tag::bvbin_tag tag, std::string value) {
      return SolverContext::operator()(tag, std::any(value));
    }

    // special handling of bvhex_tag
    result_type operator()(logic::QF_BV::tag::bvhex_tag tag, std::string value) {
      return SolverContext::operator()(tag, std::any(value));
    }

    /// handling of logic::predicate (boolean variables)
    result_type operator()(::metaSMT::logic::tag::var_tag tag) {
      assert(tag.id != 0);
      typename VariableLookupT::const_iterator iter = _variables.find(tag.id);
      if (iter != _variables.end()) {
        return iter->second;
      } else {
        result_type ret = SolverContext::operator()(tag, std::any());
        _variables.insert(std::make_pair(tag.id, ret));
        return ret;
      }
    }

    /**
     * @brief result retrieval for logic::predicate (boolean variables)
     *
     * @pre solve() must be true before calling and no assertions/assumptions
     *      may be added in between.
     *
     * @returns a result_wrapper that is converatble to boolean (and other
     *      types) containing the assignment for var, or "X" if var is not
     *      known to the solver.
     **/
    result_wrapper read_value(logic::tag::var_tag tag) {
      assert(tag.id != 0);
      typename VariableLookupT::const_iterator iter = _variables.find(tag.id);
      if (iter != _variables.end()) {
        return (*this)(tag);  // SolverContext::read_value(proto::eval(var, *this));
      } else {
        // unknown variable
        return result_wrapper(&indeterminate);
      }
    }

    /**
     * @brief result retrieval for logic::QF_BV::bitvector variables
     *
     * @pre solve() must be true before calling and no assertions/assumptions
     *      may be added in between.
     *
     * @returns a result_wrapper containing the assignment for var, or "X" if
     *      var is not known to the solver.
     **/
    result_wrapper read_value(logic::QF_BV::tag::var_tag tag) {
      assert(tag.id != 0);
      typename VariableLookupT::const_iterator iter = _variables.find(tag.id);
      if (iter != _variables.end()) {
        return (*this)(tag);  // SolverContext::read_value(proto::eval(var, *this));
      } else {
        // unknown variable
        std::vector<tribool> ret(tag.width, indeterminate);
        return result_wrapper(ret);
      }
    }

    using SolverContext::read_value;
    using SolverContext::operator();

    result_type operator()(::metaSMT::logic::QF_UF::tag::function_var_tag tag) {
      assert(tag.id != 0);
      typename VariableLookupT::const_iterator iter = _variables.find(tag.id);
      if (iter != _variables.end()) {
        return iter->second;
      } else {
        result_type ret = SolverContext::operator()(tag, std::any());
        _variables.insert(std::make_pair(tag.id, ret));
        return ret;
      }
    }

    result_type operator()(::metaSMT::logic::Array::tag::array_var_tag tag) {
      assert(tag.id != 0);
      typename VariableLookupT::const_iterator iter = _variables.find(tag.id);
      if (iter != _variables.end()) {
        return iter->second;
      } else {
        result_type ret = SolverContext::operator()(tag, std::any());
        _variables.insert(std::make_pair(tag.id, ret));
        return ret;
      }
    }

    result_type operator()(::metaSMT::logic::QF_BV::tag::var_tag tag) {
      assert(tag.id != 0);
      typename VariableLookupT::const_iterator iter = _variables.find(tag.id);
      if (iter != _variables.end()) {
        return iter->second;
      } else {
        result_type ret = SolverContext::operator()(tag, std::any());
        _variables.insert(std::make_pair(tag.id, ret));
        return ret;
      }
    }

    /*template <typename Tag>
    result_type operator()(Tag t) {
      return SolverContext::operator()(t, std::any());
    }*/

    /*template <typename Tag, typename Expr1>
    result_type operator()(Tag t, Expr1 e1) {
      return SolverContext::operator()(t, proto::eval(e1, *this));
    }*/

    template <typename Tag, typename Expr>
    result_type operator()(Tag t, std::vector<Expr> const &es) {
      std::vector<result_type> rs;
      for (unsigned u = 0; u < es.size(); ++u) {
        rs.push_back(*(this)(es[u]));
      }
      return SolverContext::operator()(t, rs);
    }

    template <typename Tag, typename Expr1, typename Expr2>
    result_type operator()(Tag t, Expr1 e1, Expr2 e2) {
      return SolverContext::operator()(t, (*this)(e1), (*this)(e2));
    }

    template <typename Tag, typename Expr1, typename Expr2, typename Expr3>
    result_type operator()(Tag t, Expr1 e1, Expr2 e2, Expr3 e3) {
      return SolverContext::operator()(t, (*this)(e1), (*this)(e2), (*this)(e3));
    }

    template <typename Tag, typename Expr1, typename Expr2, typename Expr3, typename Expr4>
    result_type operator()(Tag t, Expr1 e1, Expr2 e2, Expr3 e3, Expr4 e4) {
      return SolverContext::operator()(t, (*this)(e1), (*this)(e2), (*this)(e3), (*this)(e4));
    }

    template <typename Tag>
    typename std::enable_if<Evaluator<Tag>::value, result_type>::type operator()(Tag const &t) {
      return Evaluator<Tag>::eval(*this, t);
    }

    template <typename Tag>
    typename std::enable_if<!Evaluator<Tag>::value, result_type>::type operator()(Tag const &t) {
      return SolverContext::operator()(t, std::any());
    }

    result_type operator()(result_type r) { return r; }

    unsigned get_bv_width(result_type const &e) { return SolverContext::get_bv_width(e); }

    void command(assertion_cmd const &, result_type e) { SolverContext::assertion(e); }

    void command(assumption_cmd const &, result_type e) { SolverContext::assumption(e); }

    // void command(set_option_cmd const &, std::string const &key, std::string const &value) {
    //   opt.set(key, value);
    //   typedef typename mpl::if_<
    //       /* if   = */ typename features::supports<SolverContext, set_option_cmd>::type,
    //       /* then = */ option::SetOptionCommand, /* else = */ option::NOPCommand>::type Command;
    //   Command::template action(static_cast<SolverContext &>(*this), opt, key, value);
    // }

    std::string command(get_option_cmd const &, std::string const &key) { return opt.get(key); }

    std::string command(get_option_cmd const &, std::string const &key, std::string const &default_value) {
      return opt.get(key, default_value);
    }

    using SolverContext::command;

   private:
    typedef typename std::unordered_map<unsigned, result_type> VariableLookupT;
    VariableLookupT _variables;
    Options opt;

    // disable copying DirectSolvers;
    DirectSolver_Context(DirectSolver_Context const &);
    DirectSolver_Context &operator=(DirectSolver_Context const &);
  };

  namespace features {
    template <typename Context, typename Feature>
    struct supports<DirectSolver_Context<Context>, Feature> : supports<Context, Feature>::type {};

    template <typename Context>
    struct supports<DirectSolver_Context<Context>, assertion_cmd> : std::true_type {};

    template <typename Context>
    struct supports<DirectSolver_Context<Context>, assumption_cmd> : std::true_type {};

    template <typename Context>
    struct supports<DirectSolver_Context<Context>, get_option_cmd> : std::true_type {};

    template <typename Context>
    struct supports<DirectSolver_Context<Context>, set_option_cmd> : std::true_type {};
  }  // namespace features

  template <typename SolverType>
  typename DirectSolver_Context<SolverType>::result_type evaluate(
      DirectSolver_Context<SolverType> &, typename DirectSolver_Context<SolverType>::result_type r) {
    return r;
  }

  template <typename SolverType, typename Tag, typename Expr>
  typename DirectSolver_Context<SolverType>::result_type evaluate(DirectSolver_Context<SolverType> &ctx, Tag tag,
                                                                  Expr const &e) {
    return ctx(tag, e);
  }

  template <typename SolverType>
  bool solve(DirectSolver_Context<SolverType> &ctx) {
    return ctx.solve();
  }

  /*template <typename SolverType>
  result_wrapper read_value(DirectSolver_Context<SolverType> &ctx, logic::QF_BV::bitvector const &var) {
    return ctx.read_value(var);
  }

  template <typename SolverType>
  result_wrapper read_value(DirectSolver_Context<SolverType> &ctx, logic::predicate const &var) {
    return ctx.read_value(var);
  }*/

  template <typename SolverType>
  result_wrapper read_value(DirectSolver_Context<SolverType> &ctx,
                            typename DirectSolver_Context<SolverType>::result_type var) {
    return ctx.read_value(var);
  }

}  // namespace metaSMT

//  vim: ft=cpp:ts=2:sw=2:expandtab
