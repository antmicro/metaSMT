#pragma once

#include <type_traits>

namespace metaSMT {
  namespace features {

    /**
     *  This template provides a compile-time way
     *  to check for supported features of specific contexts.
     *  e.g. to declare native supported apis.
     *
     **/
    template <typename Context, typename Feature>
    struct supports : std::false_type {};

  }  // namespace features
}  // namespace metaSMT
