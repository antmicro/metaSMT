#include "../metaSMT/impl/_var_id.hpp"

#include <atomic>
#define var_id_type std::atomic_uint

unsigned metaSMT::impl::new_var_id() {
  static var_id_type _id(0u);
  ++_id;
  return _id;
}
