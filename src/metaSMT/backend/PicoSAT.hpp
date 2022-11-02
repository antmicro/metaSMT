#pragma once

#include "../Features.hpp"
#include "../result_wrapper.hpp"
#include "../tags/SAT.hpp"

extern "C" {
#include <picosat.h>
}
#include <any>
#include <exception>
#include <iostream>
#include <vector>

namespace metaSMT {
  // Forward declaration
  struct addclause_cmd;
  namespace features {
    struct addclause_api;
  }

  namespace solver {
    class PicoSAT {
     public:
      typedef SAT::tag::lit_tag result_type;
      PicoSAT() {
        //          if ( in_use == USED )
        //            throw std:runtime_error ("Picosat already in use. Only single instances supported.");

        picosat_init();
        //          in_use = USED;
      }

      ~PicoSAT() {
        picosat_reset();
        //         in_use = UNUSED;
      }

      int toLit(result_type lit) { return lit.id; }

      void clause(std::vector<result_type> const& clause) {
        for (result_type const& lit : clause) picosat_add(toLit(lit));
        picosat_add(0);
      }

      void command(addclause_cmd const&, std::vector<result_type> const& cls) { clause(cls); }

      void assertion(result_type lit) {
        picosat_add(toLit(lit));
        picosat_add(0);
      }

      void assumption(result_type lit) { picosat_assume(toLit(lit)); }

      bool solve() {
        switch (picosat_sat(-1)) {
          case PICOSAT_UNSATISFIABLE:
            return false;
          case PICOSAT_SATISFIABLE:
            return true;
          case PICOSAT_UNKNOWN:
            throw std::runtime_error("unknown return type of picosat_sat ");
          default:
            throw std::runtime_error("unsupported return type of picosat_sat ");
        }
      }

      result_wrapper read_value(result_type lit) {
        switch (picosat_deref(toLit(lit))) {
          case -1:
            return result_wrapper('0');
          case 1:
            return result_wrapper('1');
          case 0:
            return result_wrapper('X');
          default:
            std::cerr << "Unknown result." << std::endl;
            return result_wrapper('X');
        }
      }

     private:
      //         enum { UNUSED, USED };
      //         static int in_use = UNUSED;
    };
  }  // namespace solver

  namespace features {
    template <>
    struct supports<solver::PicoSAT, features::addclause_api> : std::true_type {};
  }  // namespace features

}  // namespace metaSMT
// vim: ts=2 sw=2 et
