#pragma once

#include <cuddObj.hh>

#include "../result_wrapper.hpp"
#include "../tags/Logic.hpp"

namespace metaSMT {
  namespace solver {
    namespace predtags = ::metaSMT::logic::tag;

    class CUDD_Context {
      struct CUDDAssertion : public std::runtime_error {
        CUDDAssertion(const char* what) : std::runtime_error(what) {}
      };

      static void _cudd_error(std::string what) { throw CUDDAssertion(what.c_str()); }

     public:
      CUDD_Context() {
        _manager.setHandler(&CUDD_Context::_cudd_error);
        _manager.AutodynEnable(CUDD_REORDER_SIFT);
        _assertions = _manager.bddOne();
        _assumptions = _manager.bddOne();
      }

      typedef BDD result_type;

      void assertion(result_type e) { _assertions &= e; }

      void assumption(result_type e) { _assumptions &= e; }

      void writeDotFile(std::string const& filename) {
#ifdef _CPPCUDD  // this is defined in CUDD 2.4.x but not in 3.0.0
        BDDvector bvec(3, &_manager, NULL);
#else
        std::vector<BDD> bvec(3, _manager.bddOne());
#endif
        bvec[0] = _assertions & _assumptions;
        bvec[1] = _assertions;
        bvec[2] = _assumptions;
        char comple[] = "complete";
        char assert[] = "assertions";
        char assume[] = "assumptions";
        char* names[] = {comple, assert, assume};
        FILE* fp = fopen(filename.c_str(), "w");
#ifdef _CPPCUDD
        bvec.DumpDot(0, names, fp);
#else
        _manager.DumpDot(bvec, 0, names, fp);
#endif
        fclose(fp);
      }

      bool solve() {
        _solution.clear();
        BDD complete = _assertions & _assumptions;
        bool ret = complete != _manager.bddZero();
        _assumptions = _manager.bddOne();
        if (ret) {
          unsigned size = _manager.ReadSize();
          char* buf = new char[size];
          complete.PickOneCube(buf);
          _solution.resize(size);
          for (unsigned i = 0; i < size; ++i) {
            if (buf[i] == 2) {
              _solution[i] = indeterminate;
            } else {
              _solution[i] = buf[i];
            }
          }
          delete[] buf;
        }
        return ret;
      }

      result_wrapper read_value(result_type var) {
        assert(var.NodeReadIndex() < _solution.size());
        return result_wrapper(_solution[var.NodeReadIndex()]);
      }

      result_type operator()(predtags::var_tag const&, std::any) { return _manager.bddVar(); }

      result_type operator()(predtags::false_tag, std::any) { return _manager.bddZero(); }

      result_type operator()(predtags::true_tag, std::any) { return _manager.bddOne(); }

      result_type operator()(predtags::not_tag, result_type a) { return !a; }

      result_type operator()(predtags::equal_tag, result_type a, result_type b) { return !(a ^ b); }

      result_type operator()(predtags::nequal_tag, result_type a, result_type b) { return a ^ b; }

      result_type operator()(predtags::and_tag, result_type a, result_type b) { return a & b; }

      result_type operator()(predtags::nand_tag, result_type a, result_type b) { return !(a & b); }

      result_type operator()(predtags::xor_tag, result_type a, result_type b) { return a ^ b; }

      result_type operator()(predtags::xnor_tag, result_type a, result_type b) { return !(a ^ b); }

      result_type operator()(predtags::implies_tag, result_type a, result_type b) { return !a | b; }

      result_type operator()(predtags::or_tag, result_type a, result_type b) { return a | b; }

      result_type operator()(predtags::nor_tag, result_type a, result_type b) { return !(a | b); }

      result_type operator()(predtags::distinct_tag, result_type a, result_type b) { return a ^ b; }

      result_type operator()(predtags::ite_tag, result_type a, result_type b, result_type c) { return a.Ite(b, c); }

      ////////////////////////
      // Fallback operators //
      ////////////////////////

      template <typename TagT>
      result_type operator()(TagT, std::any) {
        assert(false && "fallback op0 called");
        return _manager.bddZero();
      }

      template <typename TagT>
      result_type operator()(TagT, result_type) {
        assert(false && "fallback op1 called");
        return _manager.bddZero();
      }

      template <typename TagT, typename T1, typename T2>
      result_type operator()(TagT, T1, T2) {
        assert(false && "fallback op2 called");
        return _manager.bddZero();
      }

      template <typename TagT, typename T1, typename T2, typename T3>
      result_type operator()(TagT, T1, T2, T3) {
        assert(false && "fallback op3 called");
        return _manager.bddZero();
      }

      /* pseudo command */
      void command(CUDD_Context const&){};

     protected:
      Cudd _manager;
      BDD _assertions;
      BDD _assumptions;
      std::vector<tribool> _solution;
    };

  }  // namespace solver
}  // namespace metaSMT

//  vim: ft=cpp:ts=2:sw=2:expandtab
