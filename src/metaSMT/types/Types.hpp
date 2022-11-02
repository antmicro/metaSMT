#pragma once

#include <variant>

#include "Array.hpp"
#include "BitVector.hpp"
#include "Boolean.hpp"

namespace metaSMT {
  namespace type {
    typedef std::variant<Boolean, BitVector, type::Array> any_type;
  }
}  // namespace metaSMT
