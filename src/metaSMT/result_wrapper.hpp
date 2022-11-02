#pragma once

#include <functional>
#include <limits>
#include <optional>
#include <variant>
#include <vector>

#include "tribool.hpp"
#include "util.hpp"

#pragma warning(disable : 4800)
#pragma warning(disable : 4146)

namespace metaSMT {
  /**
   * return value wrapper
   *
   */
  class result_wrapper {
    // converter types
    struct as_vector_tribool {
      typedef std::vector<tribool> result_type;

      result_type operator()(result_type const& v) const { return v; }
      result_type operator()(tribool const& t) const {
        result_type ret(1, t);
        return ret;
      }

      result_type operator()(bool b) const {
        result_type ret(1);
        ret[0] = b;
        return ret;
      }

      result_type operator()(std::vector<bool> const& vb) const {
        result_type ret(vb.size());
        for (unsigned i = 0; i < vb.size(); ++i) {
          ret[i] = vb[i];
        }
        return ret;
      }

      result_type operator()(std::string const& s) const {
        unsigned size = s.size();
        result_type ret(size);
        for (unsigned i = 0; i < size; ++i) {
          switch (s[size - i - 1]) {
            case '0':
              ret[i] = false;
              break;
            case '1':
              ret[i] = true;
              break;
            default:
              ret[i] = indeterminate;
          }
        }
        return ret;
      }
    };

    struct as_tribool {
      typedef tribool result_type;

      result_type operator()(result_type const& v) const { return v; }
      result_type operator()(std::vector<tribool> const& t) const {
        tribool ret = false;
        for (unsigned i = 0; i < t.size(); ++i) {
          ret = ret || t[i];
        }
        return ret;
      }

      result_type operator()(bool b) const { return b; }

      result_type operator()(std::vector<bool> const& vb) const {
        for (unsigned i = 0; i < vb.size(); ++i) {
          if (vb[i]) return true;
        }
        return false;
      }

      result_type operator()(std::string const& s) const {
        // true if any bit is true;
        tribool ret = false;
        for (unsigned i = 0; i < s.size(); ++i) {
          switch (s[i]) {
            case '0':
              break;
            case '1':
              return true;
            default:
              ret = indeterminate;
          }
        }
        return ret;
      }
    };

    struct as_vector_bool {
      typedef std::vector<bool> result_type;

      result_type operator()(result_type const& v) const { return v; }

      result_type operator()(tribool t) const {
        result_type ret(1);
        ret[0] = (bool)t;
        return ret;
      }

      result_type operator()(bool b) const {
        result_type ret(1);
        ret[0] = b;
        return ret;
      }

      result_type operator()(std::vector<tribool> vt) const {
        result_type ret(vt.size());
        for (unsigned i = 0; i < vt.size(); ++i) ret[i] = (bool)vt[i];
        return ret;
      }

      result_type operator()(std::string s) const {
        result_type ret(s.size());
        for (unsigned i = 0; i < s.size(); ++i) ret[i] = (s[s.size() - i - 1] == '1');
        return ret;
      }
    };

    struct as_string {
      typedef std::string result_type;

      result_type operator()(result_type const& v) const { return v; }

      result_type operator()(bool b) const { return b ? "1" : "0"; }

      result_type operator()(tribool val) const {
        if (indeterminate(val)) {
          return "X";
        } else {
          return val ? "1" : "0";
        }
      }

      result_type operator()(std::vector<tribool> val) const {
        unsigned size = val.size();
        std::string s(size, '\0');
        for (unsigned i = 0; i < size; ++i) {
          if (indeterminate(val[i])) {
            s[size - i - 1] = 'X';
          } else {
            s[size - i - 1] = val[i] ? '1' : '0';
          }
        }
        return s;
      }

      result_type operator()(std::vector<bool> val) const {
        unsigned size = val.size();
        std::string s(size, '\0');
        for (unsigned i = 0; i < size; ++i) {
          s[size - i - 1] = val[i] ? '1' : '0';
        }
        return s;
      }
    };

    struct check_if_X {
      typedef bool result_type;

      template <typename T>
      result_type operator()(T const&) const {
        return false;
      }

      result_type operator()(tribool val) const { return indeterminate(val); }

      result_type operator()(std::vector<tribool> val) const {
        unsigned size = val.size();
        for (unsigned i = 0; i < size; ++i) {
          if (indeterminate(val[i])) return true;
        }
        return false;
      }

      result_type operator()(std::string val) const {
        unsigned size = val.size();
        for (unsigned i = 0; i < size; ++i) {
          if (val[i] == 'x' || val[i] == 'X') return true;
        }
        return false;
      }
    };

   public:
    typedef std::variant<bool, std::vector<tribool>, std::vector<bool>, std::string, tribool> result_type;

   public:
    result_wrapper() : r(std::string("X")) {}
    result_wrapper(result_type r) : r(r) {}
    result_wrapper(tribool t) : r(t) {}
    result_wrapper(bool b) : r(tribool(b)) {}
    result_wrapper(std::string const& s) : r(toupper(s)) {}
    result_wrapper(const char* s) : r(toupper(s)) {}
    result_wrapper(const char c) : r(c == '1' ? tribool(true) : (c == '0' ? tribool(false) : tribool(indeterminate))) {}

    result_wrapper(uint64_t value, unsigned width) {
      std::string s(width, '0');
      for (unsigned i = 0; i < width; i++) {
        if (value & 1) s[width - i - 1] = '1';
        value >>= 1;
      }
      r = s;
    }

    operator std::vector<bool>() const { return std::visit(as_vector_bool(), r); }

    operator std::vector<tribool>() const { return std::visit(as_vector_tribool(), r); }

    operator std::string() const { return std::visit(as_string(), r); }

    result_wrapper& throw_if_X() {
      if (std::visit(check_if_X(), r)) {
        throw std::string("contains X");
      }
      return *this;
    }

    typedef std::optional<std::function<bool()>> Rng;

    result_wrapper& randX(Rng rng = Rng()) {
      _rng = rng;
      return *this;
    }

    bool random_bit() const { return (*_rng)(); }

    /**
     * For signed type with smaller bit-width than needed, the sign is kept and the rest is truncated.
     * The result of this operator might thus be different from the compiler's behavior as a signed
     * downcast is implementation-defined.
     */
    template <typename Integer>
    operator Integer() const {
      Integer ret = 0;
      std::string val = *this;
      if (std::is_signed<Integer>::value && val[0] == '1') ret = static_cast<Integer>(-1);
      unsigned first = 0;
      if (std::numeric_limits<Integer>::digits < val.size())  // downcasting
        first = val.size() - std::numeric_limits<Integer>::digits;
      for (unsigned i = 0; i < val.size() && i < std::numeric_limits<Integer>::digits; ++i) {
        ret <<= static_cast<Integer>(1);
        switch (val[first + i]) {
          case '0':
            break;
          case '1':
            ret |= Integer(1);
            break;
          default:
            ret |= Integer(_rng && random_bit());
        }
      }
      return ret;
    }

    operator bool() const {
      tribool b = *this;
      if (b)
        return true;
      else if (!b)
        return false;
      else
        return _rng && random_bit();
    }

    operator tribool() const { return std::visit(as_tribool(), r); }

    friend std::ostream& operator<<(std::ostream& out, result_wrapper const& rw) {
      std::string o = rw;
      out << o;
      return out;
    }

   protected:
    result_type r;
    Rng _rng;
  };

}  // namespace metaSMT

//  vim: ft=cpp:ts=2:sw=2:expandtab
