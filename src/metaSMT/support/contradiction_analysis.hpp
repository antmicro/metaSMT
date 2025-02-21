#pragma once

#include <iostream>
#include <metaSMT/support/cardinality.hpp>

using namespace metaSMT::logic;
namespace metaSMT {

  /**
   * @brief Implications are evaluated by using a Context and added to
   * a map.
   *
   * \tparam Context a metaSMT context, i.e. DirectSolver_Context<...>
   **/
  template <typename Context>
  struct InsertImplications {
    Context &ctx;
    typename Context::result_type &c;
    std::map<unsigned, bool> &s;
    InsertImplications(Context &ctx, std::map<unsigned, bool> &s, typename Context::result_type &c)
        : ctx(ctx), c(c), s(s) {}

    /*
    template <typename Expr>
    void operator()(Expr e) const {
      bool si = new_variable();
      c = ctx(tag::and_tag{}, c, implies(si, e));
      s.insert(std::make_pair(s.size(), si));
    }
    */
  }; /* detail */

  /**
   * @brief Analyze contradictions of constraints
   *
   * contradiction_analysis takes several constraints given in a Tuple
   * or in a vector and analyzes them for conflicts
   *
   * If no conflicts exist, an empty vector of vector is returned. Otherwise the conflicts are handled separately and
   *dependecies between them are detected. These are also returned in a vector of vectors.
   *
   * \param ctx is a metaSMT Context
   * \param t a tuple of constraints, instead of the tuple you can take a vector
   * \return a set conflicts, where each conflict is identified by indices of
   *         the constraints involved in the conflict.
   *
   *@ingroup Support
   *@defgroup Analyze Analyze contradictions
   *@{
   *
   */

  /**
   * @brief ...
   */
  template <typename Context, typename Tuple>
  std::vector<std::vector<unsigned> > contradiction_analysis(Context &ctx, Tuple t) {
    std::vector<std::vector<unsigned> > results;  // Fürs Endergebnis
    std::map<unsigned, bool> s;                   // Vector für si's 1.version
    std::vector<bool> sv;                         // s im vector sv

    typename Context::result_type c = ctx(true);

    InsertImplications<Context> impl(ctx, s, c);
    for_each(t, impl);
    return analyze_conflicts(ctx, s, c, results);
  }  // end of method contradiction_analysis with tuple

  template <typename Context>
  std::vector<std::vector<unsigned> > contradiction_analysis(Context &ctx,
                                                             std::vector<typename Context::result_type> t) {
    std::vector<std::vector<unsigned> > results;  // Fürs Endergebnis
    std::map<unsigned, bool> s;                   // Vector für si's 1.version

    typename Context::result_type c = ctx(true);

    InsertImplications<Context> impl(ctx, s, c);
    std::for_each(t.begin(), t.end(), impl);

    return analyze_conflicts(ctx, s, c, results);
  }  // end of method contradiction_analysis with vector

  template <typename Context, typename BooleanType>
  std::vector<std::vector<unsigned> > analyze_conflicts(Context &ctx, std::map<unsigned, BooleanType> s,
                                                        typename Context::result_type c,
                                                        std::vector<std::vector<unsigned> > results) {
    typedef std::pair<unsigned, BooleanType> Si_Pair;
    std::vector<BooleanType> sv;
    metaSMT::assumption(ctx, c);
    for (Si_Pair si : s) {
      metaSMT::assumption(ctx, si.second);
    }

    if (solve(ctx)) {  // problem is unsat, no conflict
      return results;
    }

    // insert from map into vector
    for (unsigned it = 0; it < s.size(); it++) {
      sv.push_back(s[it]);  // ::iterator
    }

    if (sv.size() == 1) {
      // the only constraint is the reasons for the conflict
      results.push_back(std::vector<unsigned>(1 /*size*/, 0 /*constraint*/));
      return results;
    }

    std::map<unsigned, BooleanType> conflicts;
    analyze_multiple_conflict(ctx, c, s, conflicts, results);

    return results;
  }

  template <typename Context, typename BooleanType>
  void analyze_multiple_conflict(Context &ctx, typename Context::result_type c,
                                 std::map<unsigned, BooleanType> const &enable_vars,
                                 std::map<unsigned, BooleanType> const &confl_vars,
                                 std::vector<std::vector<unsigned> > &results) {
    typedef std::pair<unsigned, BooleanType> Si_Pair;

    std::vector<BooleanType> sv;

    for (Si_Pair si : enable_vars) {
      sv.push_back(si.second);
    }

    typename Context::result_type custom_c = c;
    for (Si_Pair si : confl_vars) {
      custom_c = ctx(tag::and_tag{}, custom_c, si.second);
    }

    // check custom_c is SATISFIABLE
    assumption(ctx, custom_c);
    if (!solve(ctx)) {
      // confl_vars is a complete conflict
      std::vector<unsigned> result;
      for (Si_Pair si : confl_vars) {
        result.push_back(si.first);
      }
      results.push_back(result);
      return;
    }

    const unsigned size = enable_vars.size();

    for (unsigned j = 1; j <= size; j++) {
      // prüfe ob diese formel erfüllbar ist
      metaSMT::assumption(ctx, ctx(tag::and_tag{}, cardinality_eq(ctx, sv, size - j), custom_c));

      if (!solve(ctx)) continue;

      std::map<unsigned, BooleanType> custom_enab = enable_vars;
      std::map<unsigned, BooleanType> custom_conf = confl_vars;

      // sammel alle abgeschalteten si in conflicting
      std::map<unsigned, BooleanType> conflicting;
      for (Si_Pair si : enable_vars) {
        bool tmp = read_value(ctx, si.second);
        if (!tmp) {
          conflicting.insert(si);
          custom_enab.erase(si.first);
        }
      }
      for (Si_Pair si : conflicting) {
        custom_conf.insert(si);
        analyze_multiple_conflict(ctx, c, custom_enab, custom_conf, results);
        custom_conf.erase(si.first);
      }
      // found at least one conflict
      break;
    }  // KLammer für for-schleife

  }  // end of method analyse_multiple_conflict
  /**@}*/
}  // namespace metaSMT
// vim: ts=2 sw=2 et
