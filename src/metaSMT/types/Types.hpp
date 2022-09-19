#pragma once

#include <boost/variant/variant.hpp>

#include "Array.hpp"
#include "BitVector.hpp"
#include "Boolean.hpp"

namespace metaSMT {
  namespace type {
    typedef boost::variant<Boolean, BitVector, type::Array> any_type;
  }
}  // namespace metaSMT
