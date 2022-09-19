#include "../metaSMT/impl/_var_id.hpp"

#if defined(_WIN32) || (__cplusplus > 199711L)  // essentially require C++11 on Windows

#include <atomic>
#define var_id_type std::atomic_uint

#else

#include <boost/version.hpp>
#if BOOST_VERSION >= 105500
#include <boost/atomic.hpp>
#define var_id_type boost::atomic_uint
#else
#define var_id_type unsigned;
#pragma message("Creating a new variable in metaSMT is not an atomic operation!")
#endif

#endif

unsigned metaSMT::impl::new_var_id() {
  static var_id_type _id(0u);
  ++_id;
  return _id;
}
