#pragma once

#include "Evaluator.hpp"

namespace metaSMT {

  namespace cardtags = metaSMT::logic::cardinality::tag;

  template <typename Context, typename Boolean>
  typename Context::result_type one_hot(Context &ctx, std::vector<Boolean> const &ps) {
    assert(ps.size() > 0 && "One hot encoding requires at least one input variable");

    if (ps.size() == 1) {
      return ctx(logic::tag::equal_tag{}, ps[0], ctx(true));
    }

    typename Context::result_type zero_rail = evaluate(ctx, ps[0]);
    typename Context::result_type one_rail = evaluate(ctx, Not(ps[0]));

    for (unsigned u = 1; u < ps.size() - 1; ++u) {
      zero_rail = ctx(logic::tag::ite_tag{}, ps[u], one_rail, zero_rail);
      one_rail = ctx(logic::tag::ite_tag{}, ps[u], ctx(false), one_rail);
    }

    return ctx(logic::tag::ite_tag{}, ps[ps.size() - 1], one_rail, zero_rail);
  }

  template <typename Context, typename Boolean>
  typename Context::result_type cardinality_eq(Context &ctx, std::vector<Boolean> const &ps, unsigned cardinality) {
    return ctx(cardinality::cardinality(cardtags::eq_tag(), ps, cardinality));
  }

  template <typename Context, typename Boolean>
  typename Context::result_type cardinality_geq(Context &ctx, std::vector<Boolean> const &ps, unsigned cardinality) {
    return ctx(cardinality::cardinality(cardtags::ge_tag(), ps, cardinality));
  }

  template <typename Context, typename Boolean>
  typename Context::result_type cardinality_leq(Context &ctx, std::vector<Boolean> const &ps, unsigned cardinality) {
    return ctx(cardinality::cardinality(cardtags::le_tag(), ps, cardinality));
  }

  template <typename Context, typename Boolean>
  typename Context::result_type cardinality_gt(Context &ctx, std::vector<Boolean> const &ps, unsigned cardinality) {
    return ctx(cardinality::cardinality(cardtags::gt_tag(), ps, cardinality));
  }

  template <typename Context, typename Boolean>
  typename Context::result_type cardinality_lt(Context &ctx, std::vector<Boolean> const &ps, unsigned cardinality) {
    return ctx(cardinality::cardinality(cardtags::lt_tag(), ps, cardinality));
  }
}  // namespace metaSMT
