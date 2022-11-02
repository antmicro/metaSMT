#pragma once

#include <queue>

#include "../../tags/Cardinality.hpp"
#include "object.hpp"

namespace metaSMT {
  namespace cardinality {
    namespace adder {
      namespace cardtags = logic::cardinality::tag;
      template <typename Context, typename Boolean>
      typename Context::result_type cardinality_any(Context &ctx, std::vector<Boolean> const &ps) {
        typedef typename Context::result_type result_type;

        std::queue<result_type> line0, line1;
        std::queue<result_type> *wq0 = &line0;
        std::queue<result_type> *wq1 = &line1;
        std::vector<result_type> r;

        // initialize wq0
        for (unsigned u = 0; u < ps.size(); ++u) {
          wq0->push(ctx(ps[u]));
        }

        while (!wq0->empty()) {
          while (wq0->size() >= 3) {
            result_type x = wq0->front();
            wq0->pop();
            result_type y = wq0->front();
            wq0->pop();
            result_type z = wq0->front();
            wq0->pop();
            wq0->push(ctx(logic::tag::xor_tag{}, ctx(logic::tag::xor_tag{}, x, y), z));

            wq1->push(ctx(logic::tag::ite_tag{}, x, ctx(logic::tag::or_tag{}, y, z), ctx(logic::tag::and_tag{}, y, z)));
          }

          if (wq0->size() == 2) {
            result_type x = wq0->front();
            wq0->pop();
            result_type y = wq0->front();
            wq0->pop();
            wq0->push(ctx(logic::tag::xor_tag{}, x, y));
            wq1->push(ctx(logic::tag::and_tag{}, x, y));
          }

          r.push_back(wq0->front());
          wq0->pop();

          swap(wq0, wq1);
        }

        auto n = logic::QF_BV::new_bitvector(r.size());
        for (unsigned u = 0; u < r.size(); ++u) {
          assertion(
              ctx, ctx(logic::tag::equal_tag{}, ctx(logic::QF_BV::tag::extract_tag{}, u, u, ctx(n)),
                       ctx(logic::tag::ite_tag{}, r[u], logic::QF_BV::tag::bit1_tag{}, logic::QF_BV::tag::bit0_tag{})));
        }

        return ctx(n);

        // convert result to bitvector
        // result_type n = evaluate(ctx, logic::Ite(r[0], logic::QF_BV::bit1, logic::QF_BV::bit0));
        // for ( unsigned u = 1; u < r.size(); ++u ) {
        //  n = evaluate(ctx, concat(logic::Ite(r[u], logic::QF_BV::bit1, logic::QF_BV::bit0), n));
        //}
        // return n;
      }

      template <typename Context, typename Boolean, typename Tag>
      typename Context::result_type cardinality(Context &, cardinality::Cardinality<Tag, Boolean> const &) {
        /** error: unknown tag **/
        // FIXME:
        // ASSERT_NOT((contains<cardtags::Cardinality_Tags, Tag>));
      }

      template <typename Context, typename Boolean>
      typename Context::result_type cardinality(Context &ctx,
                                                cardinality::Cardinality<cardtags::eq_tag, Boolean> const &c) {
        std::vector<Boolean> const &ps = c.ps;
        uint64_t const cardinality = c.cardinality;

        assert(ps.size() > 0 && "Equal cardinality constraint requires at least one input variable");
        unsigned const w = log2(ps.size()) + 1;
        return ctx(logic::tag::equal_tag{}, cardinality_any(ctx, ps),
                   ctx(logic::QF_BV::tag::bvuint_tag{}, cardinality, w));
      }

      template <typename Context, typename Boolean>
      typename Context::result_type cardinality(Context &ctx,
                                                cardinality::Cardinality<cardtags::le_tag, Boolean> const &c) {
        std::vector<Boolean> const &ps = c.ps;
        uint64_t const cardinality = c.cardinality;

        assert(ps.size() > 0 && "Lower equal cardinality constraint requires at least one input variable");
        unsigned const w = log2(ps.size()) + 1;
        return ctx(logic::QF_BV::tag::bvule_tag{}, cardinality_any(ctx, ps),
                   ctx(logic::QF_BV::tag::bvuint_tag{}, cardinality, w));
      }

      template <typename Context, typename Boolean>
      typename Context::result_type cardinality(Context &ctx,
                                                cardinality::Cardinality<cardtags::lt_tag, Boolean> const &c) {
        std::vector<Boolean> const &ps = c.ps;
        uint64_t const cardinality = c.cardinality;

        assert(ps.size() > 0 && "Lower than cardinality constraint requires at least one input variable");
        unsigned const w = log2(ps.size()) + 1;
        return ctx(logic::QF_BV::tag::bvult_tag{}, cardinality_any(ctx, ps),
                   ctx(logic::QF_BV::tag::bvuint_tag{}, cardinality, w));
      }

      template <typename Context, typename Boolean>
      typename Context::result_type cardinality(Context &ctx,
                                                cardinality::Cardinality<cardtags::gt_tag, Boolean> const &c) {
        std::vector<Boolean> const &ps = c.ps;
        uint64_t const cardinality = c.cardinality;

        assert(ps.size() > 0 && "Greater than cardinality constraint requires at least one input variable");
        unsigned const w = log2(ps.size()) + 1;
        return ctx(logic::QF_BV::tag::bvugt_tag{}, cardinality_any(ctx, ps),
                   ctx(logic::QF_BV::tag::bvuint_tag{}, cardinality, w));
      }

      template <typename Context, typename Boolean>
      typename Context::result_type cardinality(Context &ctx,
                                                cardinality::Cardinality<cardtags::ge_tag, Boolean> const &c) {
        std::vector<Boolean> const &ps = c.ps;
        uint64_t const cardinality = c.cardinality;

        assert(ps.size() > 0 && "Greater equal cardinality constraint requires at least one input variable");
        unsigned const w = log2(ps.size()) + 1;
        return ctx(logic::QF_BV::tag::bvuge_tag{}, cardinality_any(ctx, ps),
                   ctx(logic::QF_BV::tag::bvuint_tag{}, cardinality, w));
      }
    }  // namespace adder
  }    // namespace cardinality
}  // namespace metaSMT
