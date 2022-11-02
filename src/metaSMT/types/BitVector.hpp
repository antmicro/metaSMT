#pragma once

#include <type_traits>

namespace metaSMT {
  namespace type {

    struct BitVector {
      unsigned width;

      BitVector() {}

      BitVector(unsigned w) : width(w) {}

      template <typename STREAM>
      friend STREAM& operator<<(STREAM& out, BitVector const& self) {
        return (out << "BitVector [" << self.width << "]");
      }
    };

    /** equality of BitVector Types **/
    template <typename T>
    typename std::enable_if<std::is_same<BitVector, T>::value, bool>::type operator==(BitVector const& a, T const& b) {
      return a.width == b.width;
    }

    template <typename T>
    typename std::enable_if<!std::is_same<BitVector, T>::value, bool>::type operator==(BitVector const&, T const&) {
      return false;
    }

  }  // namespace type
}  // namespace metaSMT
