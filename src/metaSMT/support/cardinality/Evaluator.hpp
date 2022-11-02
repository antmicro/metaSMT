#pragma once
#include <metaSMT/tags/Cardinality.hpp>
#include <optional>

#include "../../API/Evaluator.hpp"
#include "adder_impl.hpp"
#include "bdd_impl.hpp"
#include "object.hpp"

namespace metaSMT {
  namespace cardinality {
    namespace cardtags = metaSMT::logic::cardinality::tag;
    template <typename Context, typename Boolean>
    std::optional<typename Context::result_type> cardinality_simplify(
        Context &ctx, cardinality::Cardinality<cardtags::eq_tag, Boolean> const &c) {
      std::vector<Boolean> const &ps = c.ps;
      unsigned const nps = ps.size();
      unsigned const cardinality = c.cardinality;
      typename Context::result_type res = ctx(true);
      if (cardinality == 0) {
        for (unsigned u = 0; u < nps; ++u) {
          res = ctx(logic::tag::and_tag{}, res, ctx(logic::tag::not_tag{}, ps[u]));
        }
        return std::optional<typename Context::result_type>(res);
      }
      if (cardinality == nps) {
        for (unsigned u = 0; u < nps; ++u) {
          res = ctx(logic::tag::and_tag{}, res, ps[u]);
        }
        return std::optional<typename Context::result_type>(res);
      }

      if (cardinality == 1 && nps == 2) {
        return std::optional<typename Context::result_type>(ctx(logic::tag::xor_tag{}, ps[0], ps[1]));
      }

      if (cardinality > nps) {
        return std::optional<typename Context::result_type>(ctx(false));
      }

      return std::optional<typename Context::result_type>();
    }
    template <typename Context, typename Boolean>
    std::optional<typename Context::result_type> cardinality_simplify(
        Context &ctx, cardinality::Cardinality<cardtags::gt_tag, Boolean> const &c) {
      std::vector<Boolean> const &ps = c.ps;
      unsigned const nps = ps.size();
      unsigned const cardinality = c.cardinality;
      typename Context::result_type res = ctx(true);

      if (nps <= cardinality) {
        return std::optional<typename Context::result_type>(ctx, false);
      }

      if (nps == cardinality + 1) {
        for (unsigned u = 0; u < nps; ++u) res = ctx(logic::tag::and_tag{}, res, ps[u]);
        return std::optional<typename Context::result_type>(res);
      }

      return std::optional<typename Context::result_type>();
    }

    template <typename Context, typename Boolean>
    std::optional<typename Context::result_type> cardinality_simplify(
        Context &ctx, cardinality::Cardinality<cardtags::ge_tag, Boolean> const &c) {
      std::vector<Boolean> const &ps = c.ps;
      unsigned const nps = ps.size();
      unsigned const cardinality = c.cardinality;
      typename Context::result_type res = ctx(true);

      if (nps < cardinality) {
        return std::optional<typename Context::result_type>(ctx(false));
      }

      if (cardinality == 0) {
        return std::optional<typename Context::result_type>(res);
      }

      if (nps == cardinality) {
        for (unsigned u = 0; u < nps; ++u) res = ctx(logic::tag::and_tag{}, res, ps[u]);
        return std::optional<typename Context::result_type>(res);
      }

      return std::optional<typename Context::result_type>();
    }

    template <typename Context, typename Boolean>
    std::optional<typename Context::result_type> cardinality_simplify(
        Context &ctx, cardinality::Cardinality<cardtags::lt_tag, Boolean> const &c) {
      std::vector<Boolean> const &ps = c.ps;
      unsigned const nps = ps.size();
      unsigned const cardinality = c.cardinality;
      typename Context::result_type res = ctx(true);

      if (nps < cardinality) {
        return std::optional<typename Context::result_type>(res);
      }
      if (cardinality == 0) {
        return std::optional<typename Context::result_type>(ctx(false));
      }
      if (cardinality == 1) {
        for (unsigned u = 0; u < nps; ++u) {
          res = ctx(logic::tag::and_tag{}, res, ctx(logic::tag::not_tag{}, ps[u]));
        }
        return std::optional<typename Context::result_type>(res);
      }

      return std::optional<typename Context::result_type>();
    }

    template <typename Context, typename Boolean>
    std::optional<typename Context::result_type> cardinality_simplify(
        Context &ctx, cardinality::Cardinality<cardtags::le_tag, Boolean> const &c) {
      std::vector<Boolean> const &ps = c.ps;
      unsigned const nps = ps.size();
      unsigned const cardinality = c.cardinality;

      typename Context::result_type res = ctx(true);
      if (nps <= cardinality) {
        return std::optional<typename Context::result_type>(res);
      }

      if (cardinality == 0) {
        for (unsigned u = 0; u < nps; ++u) {
          res = ctx(logic::tag::and_tag{}, res, ctx(logic::tag::not_tag{}, ps[u]));
        }
        return std::optional<typename Context::result_type>(res);
      }

      return std::optional<typename Context::result_type>();
    }

    // Empty Template to match. Just returns an empty optional
    template <typename Context, typename Tag, typename Boolean>
    std::optional<typename Context::result_type> cardinality_simplify(Context &,
                                                                      cardinality::Cardinality<Tag, Boolean> const &) {
      return std::optional<typename Context::result_type>();
    }
  }  // namespace cardinality

  template <typename Tag, typename Boolean>
  struct Evaluator<cardinality::Cardinality<Tag, Boolean> > : public std::true_type {
    template <typename Context>
    static typename Context::result_type eval(Context &ctx, cardinality::Cardinality<Tag, Boolean> const &c) {
      std::optional<typename Context::result_type> r = cardinality_simplify(ctx, c);
      if (r) {
        return *r;
      }

      std::string enc = c.encoding;
      if (enc == "") {
        enc = get_option(ctx, "cardinality", /* default = */ "bdd");
      }
      if (enc == "adder") {
        return cardinality::adder::cardinality(ctx, c);
      } else if (enc == "bdd") {
        return cardinality::bdd::cardinality(ctx, c);
      } else {
        assert(false && "Unknown cardinality implementation");
        throw std::exception();
      }
    }
  };  // Evaluator
}  // namespace metaSMT
