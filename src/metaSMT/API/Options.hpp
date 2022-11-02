#pragma once

#include <string>

#include "../Features.hpp"

namespace metaSMT {
  struct setup_option_map_cmd {
    typedef void result_type;
  };
  struct get_option_cmd {
    typedef std::string result_type;
  };
  struct set_option_cmd {
    typedef void result_type;
  };

  template <typename Context_>
  std::string get_option(Context_ &ctx, std::string const &key) {
    return ctx.command(get_option_cmd(), key);
  }

  template <typename Context_>
  std::string get_option(Context_ &ctx, std::string const &key, std::string const &default_value) {
    return ctx.command(get_option_cmd(), key, default_value);
  }

  template <typename Context_>
  void set_option(Context_ &ctx, std::string const &key, std::string const &value) {
    ctx.command(set_option_cmd(), key, value);
  }
}  // namespace metaSMT
