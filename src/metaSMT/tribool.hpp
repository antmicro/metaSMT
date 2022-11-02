#pragma once

#include <limits>
#include <vector>

namespace metaSMT {

  class tribool;

  /// INTERNAL ONLY
  namespace detail {
    /**
     * INTERNAL ONLY
     *
     * \brief A type used only to uniquely identify the 'indeterminate'
     * function/keyword.
     */
    struct indeterminate_t {};
  }  // end namespace detail

  /**
   * INTERNAL ONLY
   * The type of the 'indeterminate' keyword. This has the same type as the
   * function 'indeterminate' so that we can recognize when the keyword is
   * used.
   */
  typedef bool (*indeterminate_keyword_t)(tribool, detail::indeterminate_t);

  /**
   * \brief Keyword and test function for the indeterminate tribool value
   *
   * The \c indeterminate function has a dual role. It's first role is
   * as a unary function that tells whether the tribool value is in the
   * "indeterminate" state. It's second role is as a keyword
   * representing the indeterminate (just like "true" and "false"
   * represent the true and false states). If you do not like the name
   * "indeterminate", and would prefer to use a different name, see the
   * macro \c BOOST_TRIBOOL_THIRD_STATE.
   *
   * \returns <tt>x.value == tribool::indeterminate_value</tt>
   * \throws nothrow
   */
  constexpr inline bool indeterminate(tribool x, detail::indeterminate_t dummy = detail::indeterminate_t()) noexcept;

  /**
   * \brief A 3-state boolean type.
   *
   * 3-state boolean values are either true, false, or
   * indeterminate.
   */
  class tribool {
    //#if defined(BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS)
   private:
    /// INTERNAL ONLY
    struct dummy {
      void nonnull(){};
    };

    typedef void (dummy::*safe_bool)();
    //#endif

   public:
    /**
     * Construct a new 3-state boolean value with the value 'false'.
     *
     * \throws nothrow
     */
    constexpr tribool() noexcept : value(false_value) {}

    /**
     * Construct a new 3-state boolean value with the given boolean
     * value, which may be \c true or \c false.
     *
     * \throws nothrow
     */
    constexpr tribool(bool initial_value) noexcept : value(initial_value ? true_value : false_value) {}

    /**
     * Construct a new 3-state boolean value with an indeterminate value.
     *
     * \throws nothrow
     */
    constexpr tribool(indeterminate_keyword_t) noexcept : value(indeterminate_value) {}

    /**
     * Use a 3-state boolean in a boolean context. Will evaluate true in a
     * boolean context only when the 3-state boolean is definitely true.
     *
     * \returns true if the 3-state boolean is true, false otherwise
     * \throws nothrow
     */
    //#if !defined(BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS)

    constexpr explicit operator bool() const noexcept { return value == true_value; }

    //#else
    //
    //    constexpr operator safe_bool() const noexcept { return value == true_value ? &dummy::nonnull : 0; }
    //
    //#endif

    /**
     * The actual stored value in this 3-state boolean, which may be false, true,
     * or indeterminate.
     */
    enum value_t { false_value, true_value, indeterminate_value } value;
  };

  // Check if the given tribool has an indeterminate value. Also doubles as a
  // keyword for the 'indeterminate' value
  constexpr inline bool indeterminate(tribool x, detail::indeterminate_t) noexcept {
    return x.value == tribool::indeterminate_value;
  }

  /** @defgroup logical Logical operations
   */
  //@{
  /**
   * \brief Computes the logical negation of a tribool
   *
   * \returns the logical negation of the tribool, according to the
   * table:
   *  <table border=1>
   *    <tr>
   *      <th><center><code>!</code></center></th>
   *      <th/>
   *    </tr>
   *    <tr>
   *      <th><center>false</center></th>
   *      <td><center>true</center></td>
   *    </tr>
   *    <tr>
   *      <th><center>true</center></th>
   *      <td><center>false</center></td>
   *    </tr>
   *    <tr>
   *      <th><center>indeterminate</center></th>
   *      <td><center>indeterminate</center></td>
   *    </tr>
   *  </table>
   * \throws nothrow
   */
  constexpr inline tribool operator!(tribool x) noexcept {
    return x.value == tribool::false_value  ? tribool(true)
           : x.value == tribool::true_value ? tribool(false)
                                            : tribool(indeterminate);
  }

  /**
   * \brief Computes the logical conjunction of two tribools
   *
   * \returns the result of logically ANDing the two tribool values,
   * according to the following table:
   *       <table border=1>
   *           <tr>
   *             <th><center><code>&amp;&amp;</code></center></th>
   *             <th><center>false</center></th>
   *             <th><center>true</center></th>
   *             <th><center>indeterminate</center></th>
   *           </tr>
   *           <tr>
   *             <th><center>false</center></th>
   *             <td><center>false</center></td>
   *             <td><center>false</center></td>
   *             <td><center>false</center></td>
   *           </tr>
   *           <tr>
   *             <th><center>true</center></th>
   *             <td><center>false</center></td>
   *             <td><center>true</center></td>
   *             <td><center>indeterminate</center></td>
   *           </tr>
   *           <tr>
   *             <th><center>indeterminate</center></th>
   *             <td><center>false</center></td>
   *             <td><center>indeterminate</center></td>
   *             <td><center>indeterminate</center></td>
   *           </tr>
   *       </table>
   * \throws nothrow
   */
  constexpr inline tribool operator&&(tribool x, tribool y) noexcept {
    return (static_cast<bool>(!x) || static_cast<bool>(!y))
               ? tribool(false)
               : ((static_cast<bool>(x) && static_cast<bool>(y)) ? tribool(true) : indeterminate);
  }

  /**
   * \overload
   */
  constexpr inline tribool operator&&(tribool x, bool y) noexcept { return y ? x : tribool(false); }

  /**
   * \overload
   */
  constexpr inline tribool operator&&(bool x, tribool y) noexcept { return x ? y : tribool(false); }

  /**
   * \overload
   */
  constexpr inline tribool operator&&(indeterminate_keyword_t, tribool x) noexcept {
    return !x ? tribool(false) : tribool(indeterminate);
  }

  /**
   * \overload
   */
  constexpr inline tribool operator&&(tribool x, indeterminate_keyword_t) noexcept {
    return !x ? tribool(false) : tribool(indeterminate);
  }

  /**
   * \brief Computes the logical disjunction of two tribools
   *
   * \returns the result of logically ORing the two tribool values,
   * according to the following table:
   *       <table border=1>
   *           <tr>
   *             <th><center><code>||</code></center></th>
   *             <th><center>false</center></th>
   *             <th><center>true</center></th>
   *             <th><center>indeterminate</center></th>
   *           </tr>
   *           <tr>
   *             <th><center>false</center></th>
   *             <td><center>false</center></td>
   *             <td><center>true</center></td>
   *             <td><center>indeterminate</center></td>
   *           </tr>
   *           <tr>
   *             <th><center>true</center></th>
   *             <td><center>true</center></td>
   *             <td><center>true</center></td>
   *             <td><center>true</center></td>
   *           </tr>
   *           <tr>
   *             <th><center>indeterminate</center></th>
   *             <td><center>indeterminate</center></td>
   *             <td><center>true</center></td>
   *             <td><center>indeterminate</center></td>
   *           </tr>
   *       </table>
   *  \throws nothrow
   */
  constexpr inline tribool operator||(tribool x, tribool y) noexcept {
    return (static_cast<bool>(!x) && static_cast<bool>(!y))
               ? tribool(false)
               : ((static_cast<bool>(x) || static_cast<bool>(y)) ? tribool(true) : tribool(indeterminate));
  }

  /**
   * \overload
   */
  constexpr inline tribool operator||(tribool x, bool y) noexcept { return y ? tribool(true) : x; }

  /**
   * \overload
   */
  constexpr inline tribool operator||(bool x, tribool y) noexcept { return x ? tribool(true) : y; }

  /**
   * \overload
   */
  constexpr inline tribool operator||(indeterminate_keyword_t, tribool x) noexcept {
    return x ? tribool(true) : tribool(indeterminate);
  }

  /**
   * \overload
   */
  constexpr inline tribool operator||(tribool x, indeterminate_keyword_t) noexcept {
    return x ? tribool(true) : tribool(indeterminate);
  }
  //@}

  /**
   * \brief Compare tribools for equality
   *
   * \returns the result of comparing two tribool values, according to
   * the following table:
   *       <table border=1>
   *          <tr>
   *            <th><center><code>==</code></center></th>
   *            <th><center>false</center></th>
   *            <th><center>true</center></th>
   *            <th><center>indeterminate</center></th>
   *          </tr>
   *          <tr>
   *            <th><center>false</center></th>
   *            <td><center>true</center></td>
   *            <td><center>false</center></td>
   *            <td><center>indeterminate</center></td>
   *          </tr>
   *          <tr>
   *            <th><center>true</center></th>
   *            <td><center>false</center></td>
   *            <td><center>true</center></td>
   *            <td><center>indeterminate</center></td>
   *          </tr>
   *          <tr>
   *            <th><center>indeterminate</center></th>
   *            <td><center>indeterminate</center></td>
   *            <td><center>indeterminate</center></td>
   *            <td><center>indeterminate</center></td>
   *          </tr>
   *      </table>
   * \throws nothrow
   */
  constexpr inline tribool operator==(tribool x, tribool y) noexcept {
    return (indeterminate(x) || indeterminate(y)) ? indeterminate : ((x && y) || (!x && !y));
  }

  /**
   * \overload
   */
  constexpr inline tribool operator==(tribool x, bool y) noexcept { return x == tribool(y); }

  /**
   * \overload
   */
  constexpr inline tribool operator==(bool x, tribool y) noexcept { return tribool(x) == y; }

  /**
   * \overload
   */
  constexpr inline tribool operator==(indeterminate_keyword_t, tribool x) noexcept {
    return tribool(indeterminate) == x;
  }

  /**
   * \overload
   */
  constexpr inline tribool operator==(tribool x, indeterminate_keyword_t) noexcept {
    return tribool(indeterminate) == x;
  }

  /**
   * \brief Compare tribools for inequality
   *
   * \returns the result of comparing two tribool values for inequality,
   * according to the following table:
   *       <table border=1>
   *           <tr>
   *             <th><center><code>!=</code></center></th>
   *             <th><center>false</center></th>
   *             <th><center>true</center></th>
   *             <th><center>indeterminate</center></th>
   *           </tr>
   *           <tr>
   *             <th><center>false</center></th>
   *             <td><center>false</center></td>
   *             <td><center>true</center></td>
   *             <td><center>indeterminate</center></td>
   *           </tr>
   *           <tr>
   *             <th><center>true</center></th>
   *             <td><center>true</center></td>
   *             <td><center>false</center></td>
   *             <td><center>indeterminate</center></td>
   *           </tr>
   *           <tr>
   *             <th><center>indeterminate</center></th>
   *             <td><center>indeterminate</center></td>
   *             <td><center>indeterminate</center></td>
   *             <td><center>indeterminate</center></td>
   *           </tr>
   *       </table>
   * \throws nothrow
   */
  constexpr inline tribool operator!=(tribool x, tribool y) noexcept {
    return (indeterminate(x) || indeterminate(y)) ? indeterminate : !((x && y) || (!x && !y));
  }

  /**
   * \overload
   */
  constexpr inline tribool operator!=(tribool x, bool y) noexcept { return x != tribool(y); }

  /**
   * \overload
   */
  constexpr inline tribool operator!=(bool x, tribool y) noexcept { return tribool(x) != y; }

  /**
   * \overload
   */
  constexpr inline tribool operator!=(indeterminate_keyword_t, tribool x) noexcept {
    return tribool(indeterminate) != x;
  }

  /**
   * \overload
   */
  constexpr inline tribool operator!=(tribool x, indeterminate_keyword_t) noexcept {
    return x != tribool(indeterminate);
  }

}  // namespace metaSMT

//  vim: ft=cpp:ts=2:sw=2:expandtab
