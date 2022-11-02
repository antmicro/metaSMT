#pragma once

#include <type_traits>

namespace metaSMT {
  namespace type {

    struct Boolean {
      template <typename STREAM>
      friend STREAM& operator<<(STREAM& out, Boolean const&) {
        return (out << "Boolean");
      }
    };

    /** equality of Boolean Types **/
    template <typename T>
    bool operator==(Boolean const&, T const&) {
      return std::is_same_v<T, Boolean>;
    }

  }  // namespace type
}  // namespace metaSMT
