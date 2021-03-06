/* A less simple result type
(C) 2017 Niall Douglas <http://www.nedproductions.biz/> (59 commits)
File Created: June 2017


Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License in the accompanying file
Licence.txt or at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


Distributed under the Boost Software License, Version 1.0.
(See accompanying file Licence.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef OUTCOME_OUTCOME_HPP
#define OUTCOME_OUTCOME_HPP

#include "result.hpp"

#include <memory>

OUTCOME_V2_NAMESPACE_BEGIN

//! Thrown when you try to access state in a `outcome<T, EC, E>` which isn't present.
class OUTCOME_SYMBOL_VISIBLE bad_outcome_access : public std::logic_error
{
public:
  bad_outcome_access(const char *what)
      : std::logic_error(what)
  {
  }
};

//! Placeholder type to indicate there is no payload type
struct no_payload_type
{
  no_payload_type() = delete;
};
//! Placeholder type to indicate there is no exception type
struct no_exception_type
{
  no_exception_type() = delete;
};

namespace impl
{
  //! The payload observers implementation of `outcome<R, S, P>`. Only appears separate due to standardese limitations.
  template <class Base, class R, class S, class P, class NoValuePolicy> class outcome_payload_observers : public Base
  {
  public:
    using payload_type = P;
    using Base::Base;

    /// \output_section Narrow state observers
    /*! Access payload without runtime checks.
    \preconditions The outcome to have an payload state, otherwise it is undefined behaviour.
    \returns Reference to the held `payload_type` according to overload.
    \group assume_payload
    */
    inline constexpr payload_type &assume_payload() & noexcept;
    /// \group assume_payload
    inline constexpr const payload_type &assume_payload() const &noexcept;
    /// \group assume_payload
    inline constexpr payload_type &&assume_payload() && noexcept;
    /// \group assume_payload
    inline constexpr const payload_type &&assume_payload() const &&noexcept;

    /// \output_section Wide state observers
    /*! Access payload with runtime checks.
    \returns Reference to the held `payload_type` according to overload.
    \requires The outcome to have an payload state, else whatever `NoValuePolicy` says ought to happen.
    \group payload
    */
    inline constexpr payload_type &payload() &;
    /// \group payload
    inline constexpr const payload_type &payload() const &;
    /// \group payload
    inline constexpr payload_type &&payload() &&;
    /// \group payload
    inline constexpr const payload_type &&payload() const &&;
  };

  template <class Base, class R, class S, class NoValuePolicy> class outcome_payload_observers<Base, R, S, void, NoValuePolicy> : public Base
  {
  public:
    using Base::Base;
    /// \output_section Narrow state observers
    /*! Access payload without runtime checks.
    */
    constexpr void assume_payload() const noexcept { NoValuePolicy::narrow_payload_check(this); }
    /// \output_section Wide state observers
    /*! Access payload with runtime checks.
    \requires The outcome to have an payload state, else whatever `NoValuePolicy` says ought to happen.
    */
    constexpr void payload() const { NoValuePolicy::wide_payload_check(this); }
  };

  //! The exception observers implementation of `outcome<R, S, P>`. Only appears separate due to standardese limitations.
  template <class Base, class R, class S, class P, class NoValuePolicy> class outcome_exception_observers : public Base
  {
  public:
    using exception_type = P;
    using Base::Base;

    /// \output_section Narrow state observers
    /*! Access exception without runtime checks.
    \preconditions The outcome to have an exception state, otherwise it is undefined behaviour.
    \returns Reference to the held `exception_type` according to overload.
    \group assume_exception
    */
    constexpr inline exception_type &assume_exception() & noexcept;
    /// \group assume_exception
    constexpr inline const exception_type &assume_exception() const &noexcept;
    /// \group assume_exception
    constexpr inline exception_type &&assume_exception() && noexcept;
    /// \group assume_exception
    constexpr inline const exception_type &&assume_exception() const &&noexcept;

    /// \output_section Wide state observers
    /*! Access exception with runtime checks.
    \returns Reference to the held `exception_type` according to overload.
    \requires The outcome to have an exception state, else whatever `NoValuePolicy` says ought to happen.
    \group exception
    */
    constexpr inline exception_type &exception() &;
    /// \group exception
    constexpr inline const exception_type &exception() const &;
    /// \group exception
    constexpr inline exception_type &&exception() &&;
    /// \group exception
    constexpr inline const exception_type &&exception() const &&;
  };
  template <class Base, class R, class S, class NoValuePolicy> class outcome_exception_observers<Base, R, S, void, NoValuePolicy> : public Base
  {
  public:
    using Base::Base;
    /// \output_section Narrow state observers
    /*! Access exception without runtime checks.
    */
    constexpr void assume_exception() const noexcept { NoValuePolicy::narrow_exception_check(this); }
    /// \output_section Wide state observers
    /*! Access exception with runtime checks.
    \requires The outcome to have an exception state, else whatever `NoValuePolicy` says ought to happen.
    */
    constexpr void exception() const { NoValuePolicy::wide_exception_check(this); }
  };

  //! The failure observers implementation of `outcome<R, S, P>`. Only appears separate due to standardese limitations.
  template <class Base, class R, class S, class P, class NoValuePolicy> class outcome_failure_observers : public Base
  {
  public:
    using Base::Base;
  };
  template <class Base, class R, class NoValuePolicy> class outcome_failure_observers<Base, R, std::error_code, std::exception_ptr, NoValuePolicy> : public Base
  {
  public:
    using exception_type = std::exception_ptr;
    using Base::Base;

    /// \output_section Synthesising state observers
    /*! Synthesise exception where possible.
    \returns A synthesised exception type: if excepted, `exception()`; if errored, `std::make_exception_ptr(std::system_error(error()))`;
    otherwise a default constructed exception type.
    */
    exception_type failure() const noexcept
    {
      if((this->_state._status & detail::status_have_exception) != 0)
      {
        return this->exception();
      }
      if((this->_state._status & detail::status_have_error) != 0)
      {
        return std::make_exception_ptr(std::system_error(this->error()));
      }
      return exception_type();
    }
  };
}

namespace policy
{
#ifdef __cpp_exceptions
  template <class R, class S, class P> struct error_code_throw_as_system_error_exception_rethrow;
  /*! Default `outcome<R, S, P>` policy selector.
  \module Error code interpretation policy
  */
  template <class R, class S, class P>
  using default_outcome_policy = std::conditional_t<                                                                                                       //
  detail::is_same_or_constructible<std::error_code, S> && trait::is_exception_ptr<P>::value, error_code_throw_as_system_error_exception_rethrow<R, S, P>,  //
  terminate                                                                                                                                                //
  >;
#else
  template <class R, class S, class P> using default_outcome_policy = terminate;
#endif
}

template <class R, class S = std::error_code, class P = std::exception_ptr, class NoValuePolicy = policy::default_outcome_policy<R, S, P>> OUTCOME_REQUIRES(std::is_void<P>::value || std::is_default_constructible<P>::value) class outcome;

namespace detail
{
  // May be reused by outcome subclasses to save load on the compiler
  template <class value_type, class status_error_type, class error_type, class payload_exception_type, class payload_type, class exception_type> struct outcome_predicates
  {
    using result = result_predicates<value_type, status_error_type, error_type>;
    // Predicate for the implicit constructors to be available
    static constexpr bool implicit_constructors_enabled =                                                                     //
    (std::is_same<bool, std::decay_t<value_type>>::value || !is_same_or_constructible<value_type, status_error_type>)         //
    &&(std::is_same<bool, std::decay_t<value_type>>::value || !is_same_or_constructible<value_type, payload_exception_type>)  //
    &&!is_same_or_constructible<status_error_type, value_type>                                                                //
    && !is_same_or_constructible<status_error_type, payload_exception_type>                                                   //
    && !is_same_or_constructible<payload_exception_type, value_type>                                                          //
    && !is_same_or_constructible<payload_exception_type, status_error_type>;

    // Predicate for the value converting constructor to be available.
    template <class T>
    static constexpr bool enable_value_converting_constructor =  //
    implicit_constructors_enabled                                //
    &&result::template enable_value_converting_constructor<T>    //
    && !std::is_constructible<exception_type, T>::value;

    // Predicate for the error converting constructor to be available.
    template <class T>
    static constexpr bool enable_error_converting_constructor =  //
    implicit_constructors_enabled                                //
    &&result::template enable_error_converting_constructor<T>    //
    && !std::is_constructible<payload_exception_type, T>::value;

    // Predicate for the error + payload/exception constructor to be available.
    template <class T, class U>
    static constexpr bool enable_error_payload_converting_constructor =                                 //
    !is_in_place_type_t<std::decay_t<T>>::value                                                         // not in place construction
    && !std::is_constructible<value_type, T>::value && detail::is_same_or_constructible<error_type, T>  //
    && detail::is_same_or_constructible<payload_exception_type, U> && !std::is_constructible<value_type, U>::value;

    // Predicate for the error condition converting constructor to be available.
    template <class ErrorCondEnum>
    static constexpr bool enable_error_condition_converting_constructor = result::template enable_error_condition_converting_constructor<ErrorCondEnum>  //
                                                                          && !std::is_constructible<payload_exception_type, ErrorCondEnum>::value;

    // Predicate for the exception converting constructor to be available.
    template <class T>
    static constexpr bool enable_exception_converting_constructor =  //
    implicit_constructors_enabled                                    //
    && !is_in_place_type_t<std::decay_t<T>>::value                   // not in place construction
    && !std::is_constructible<value_type, T>::value && !std::is_constructible<status_error_type, T>::value && detail::is_same_or_constructible<exception_type, T>;

    // Predicate for the converting copy constructor from a compatible outcome to be available.
    template <class T, class U, class V, class W>
    static constexpr bool enable_compatible_conversion =                                                                  //
    (std::is_void<T>::value || detail::is_same_or_constructible<value_type, typename outcome<T, U, V, W>::value_type>)    // if our value types are constructible
    &&(std::is_void<U>::value || detail::is_same_or_constructible<error_type, typename outcome<T, U, V, W>::error_type>)  // if our error types are constructible
#if OUTCOME_ENABLE_POSITIVE_STATUS
    &&(std::is_void<U>::value || detail::is_same_or_constructible<status_type, typename outcome<T, U, V, W>::status_type>)  // if our status types are constructible
#endif
    &&(std::is_void<V>::value || detail::is_same_or_constructible<payload_type, typename outcome<T, U, V, W>::payload_type>)      // if our payload types are constructible
    &&(std::is_void<V>::value || detail::is_same_or_constructible<exception_type, typename outcome<T, U, V, W>::exception_type>)  // if our exception types are constructible
    ;

    // Predicate for the implicit converting inplace constructor from a compatible input to be available.
    struct disable_inplace_value_error_exception_constructor;
    template <class... Args>
    using choose_inplace_value_error_exception_constructor = std::conditional_t<                                                                                                                                                  //
    ((static_cast<int>(std::is_constructible<value_type, Args...>::value) + static_cast<int>(std::is_constructible<error_type, Args...>::value) + static_cast<int>(std::is_constructible<exception_type, Args...>::value)) > 1),  //
    disable_inplace_value_error_exception_constructor,                                                                                                                                                                            //
    std::conditional_t<                                                                                                                                                                                                           //
    std::is_constructible<value_type, Args...>::value,                                                                                                                                                                            //
    value_type,                                                                                                                                                                                                                   //
    std::conditional_t<                                                                                                                                                                                                           //
    std::is_constructible<error_type, Args...>::value,                                                                                                                                                                            //
    error_type,                                                                                                                                                                                                                   //
    std::conditional_t<                                                                                                                                                                                                           //
    std::is_constructible<exception_type, Args...>::value,                                                                                                                                                                        //
    exception_type,                                                                                                                                                                                                               //
    disable_inplace_value_error_exception_constructor>>>>;
    template <class... Args>
    static constexpr bool enable_inplace_value_error_exception_constructor =  //
    implicit_constructors_enabled && !std::is_same<choose_inplace_value_error_exception_constructor<Args...>, disable_inplace_value_error_exception_constructor>::value;
  };

  struct enable_payload_from_failure
  {
  };
  struct enable_exception_from_failure
  {
  };
  template <class T, class U, class V, typename = std::enable_if_t<!trait::is_exception_ptr<V>::value>> constexpr inline const V &extract_exception_payload_from_failure(const failure_type<U, V> &v, enable_payload_from_failure = enable_payload_from_failure()) { return v.payload; }
  template <class T, class U, class V, typename = std::enable_if_t<!trait::is_exception_ptr<V>::value>> constexpr inline V &&extract_exception_payload_from_failure(failure_type<U, V> &&v, enable_payload_from_failure = enable_payload_from_failure()) { return std::move(v.payload); }
  template <class T, class U, class V, typename = std::enable_if_t<trait::is_exception_ptr<V>::value>> constexpr inline const V &extract_exception_payload_from_failure(const failure_type<U, V> &v, enable_exception_from_failure = enable_exception_from_failure()) { return v.exception; }
  template <class T, class U, class V, typename = std::enable_if_t<trait::is_exception_ptr<V>::value>> constexpr inline V &&extract_exception_payload_from_failure(failure_type<U, V> &&v, enable_exception_from_failure = enable_exception_from_failure()) { return std::move(v.exception); }
  template <class T, class U> constexpr inline T extract_exception_payload_from_failure(const failure_type<U, void> & /*unused*/) { return T{}; }

  template <class Base, class R, class S, class P, class NoValuePolicy> using select_outcome_observers_payload_or_exception = std::conditional_t<trait::is_exception_ptr<P>::value, impl::outcome_exception_observers<Base, R, S, P, NoValuePolicy>, impl::outcome_payload_observers<Base, R, S, P, NoValuePolicy>>;
  template <class R, class S, class P, class NoValuePolicy> using select_outcome_impl2 = select_outcome_observers_payload_or_exception<impl::result_final<R, S, NoValuePolicy>, R, S, P, NoValuePolicy>;
  template <class R, class S, class P, class NoValuePolicy> using select_outcome_impl = impl::outcome_failure_observers<select_outcome_impl2<R, S, P, NoValuePolicy>, R, S, P, NoValuePolicy>;
}

namespace hooks
{
  /*! The default instantiation hook implementation called when a `outcome` is first created
  by conversion from one of its possible types. Does nothing.
  \tparam T One of `value_type`, `error_type`, `std::pair<error_type, payload_type>` or `exception_type`.

  WARNING: The compiler is permitted to elide calls to constructors, and thus this hook may not get called when you think it should!
  */
  template <class T, class U> constexpr inline void hook_outcome_construction(in_place_type_t<T> /*unused*/, U * /*unused*/) noexcept {}
  /*! The default instantiation hook implementation called when a `outcome` is created by copying
  from another `outcome` or `result`. Does nothing.
  \tparam T The type of the source.

  WARNING: The compiler is permitted to elide calls to constructors, and thus this hook may not get called when you think it should!
  */
  template <class T, class U> constexpr inline void hook_outcome_copy_construction(in_place_type_t<T> /*unused*/, U * /*unused*/) noexcept {}
  /*! The default instantiation hook implementation called when a `outcome` is created by moving
  from another `outcome` or `result`. Does nothing.
  \tparam T The type of the source.

  WARNING: The compiler is permitted to elide calls to constructors, and thus this hook may not get called when you think it should!
  */
  template <class T, class U> constexpr inline void hook_outcome_move_construction(in_place_type_t<T> /*unused*/, U * /*unused*/) noexcept {}
  /*! The default instantiation hook implementation called when a `outcome` is created by in place
  construction. Does nothing.
  \tparam T One of `value_type`, `error_type`, `std::pair<error_type, payload_type>` or `exception_type`.

  WARNING: The compiler is permitted to elide calls to constructors, and thus this hook may not get called when you think it should!
  */
  template <class T, class U> constexpr inline void hook_outcome_in_place_construction(in_place_type_t<T> /*unused*/, U * /*unused*/) noexcept {}

  template <class R, class S, class P, class NoValuePolicy, class U> constexpr inline void override_outcome_payload_exception(outcome<R, S, P, NoValuePolicy> *o, U &&v) noexcept;
}

/*! Used to return from functions (i) a value and (a positive status and/or a payload) or
(ii) no value and (a negative status and/or a payload). `constexpr` capable.
\module outcome<R, S, P> implementation
\tparam R The optional type of the successful result (use `void` to disable).
\tparam S The optional type of the status result (use `void` to disable). Must be either `void` or DefaultConstructible.
\tparam P The optional type of the payload/exception result (use `void` to disable). Must be either `void` or DefaultConstructible.
\tparam NoValuePolicy Policy on how to interpret types `S` and `P` when a wide observation of a not present value occurs.
*/
template <class R, class S, class P, class NoValuePolicy>                            //
OUTCOME_REQUIRES(std::is_void<P>::value || std::is_default_constructible<P>::value)  //
class OUTCOME_NODISCARD outcome : public detail::select_outcome_impl<R, S, P, NoValuePolicy>
{
  static_assert(std::is_void<P>::value || std::is_default_constructible<P>::value, "payload_type/exception_type must be default constructible");
  using base = detail::select_outcome_impl<R, S, P, NoValuePolicy>;
  friend NoValuePolicy;
  friend detail::select_outcome_impl2<R, S, P, NoValuePolicy>;
  template <class T, class U, class V, class W> friend class outcome;
  template <class T, class U, class V, class W> friend inline std::istream &operator>>(std::istream &s, outcome<T, U, V, W> &v);
  template <class T, class U, class V, class W> friend inline std::ostream &operator<<(std::ostream &s, const outcome<T, U, V, W> &v);
  template <class T, class U, class V, class W, class X> friend constexpr inline void hooks::override_outcome_payload_exception(outcome<T, U, V, W> *o, X &&v) noexcept;

  struct value_converting_constructor_tag
  {
  };
  struct value_status_converting_constructor_tag
  {
  };
  struct value_payload_converting_constructor_tag
  {
  };
  struct value_status_payload_converting_constructor_tag
  {
  };
  struct error_converting_constructor_tag
  {
  };
  struct error_condition_converting_constructor_tag
  {
  };
  struct error_payload_converting_constructor_tag
  {
  };
  struct exception_converting_constructor_tag
  {
  };

  struct disable_in_place_value_type
  {
  };
  struct disable_in_place_error_type
  {
  };
  struct disable_in_place_payload_exception_type
  {
  };

public:
  /// \output_section Member types
  //! The success type.
  using value_type = R;
  //! The S type configured
  using status_error_type = S;
#if OUTCOME_ENABLE_POSITIVE_STATUS
  //! The status type, always `no_status_type` if `trait::status_type_is_negative<S>` is true.
  using status_type = typename base::status_type;
  //! The failure type, always `no_error_type` if `trait::status_type_is_negative<S>` is false.
  using error_type = typename base::error_type;
#else
  //! The failure type.
  using error_type = S;
#endif
  //! The P type configured.
  using payload_exception_type = P;
  //! The payload type, always `no_payload_type` if `trait::is_exception_ptr<P>` is true.
  using payload_type = std::conditional_t<trait::is_exception_ptr<P>::value, no_payload_type, P>;
  //! The exception type, always `no_exception_type` if `trait::is_exception_ptr<P>` is false.
  using exception_type = std::conditional_t<!trait::is_exception_ptr<P>::value, no_exception_type, P>;

  //! Used to rebind this outcome to a different outcome type
  template <class T, class U = S, class V = P> using rebind = outcome<T, U, P>;

protected:
  //! Requirement predicates for outcome.
  struct predicate
  {
    using base = detail::outcome_predicates<value_type, status_error_type, error_type, payload_exception_type, payload_type, exception_type>;

    //! Predicate for the value converting constructor to be available.
    template <class T>
    static constexpr bool enable_value_converting_constructor =  //
    !std::is_same<std::decay_t<T>, outcome>::value               // not my type
    && base::template enable_value_converting_constructor<T>;

    //! Predicate for the error converting constructor to be available.
    template <class T>
    static constexpr bool enable_error_converting_constructor =  //
    !std::is_same<std::decay_t<T>, outcome>::value               // not my type
    && base::template enable_error_converting_constructor<T>;

    // Predicate for the error + payload/exception constructor to be available.
    template <class T, class U>
    static constexpr bool enable_error_payload_converting_constructor =  //
    !std::is_same<std::decay_t<T>, outcome>::value                       // not my type
    && base::template enable_error_payload_converting_constructor<T, U>;

    //! Predicate for the error condition converting constructor to be available.
    template <class ErrorCondEnum>
    static constexpr bool enable_error_condition_converting_constructor =  //
    !std::is_same<std::decay_t<ErrorCondEnum>, outcome>::value             // not my type
    && base::template enable_error_condition_converting_constructor<ErrorCondEnum>;

    // Predicate for the exception converting constructor to be available.
    template <class T>
    static constexpr bool enable_exception_converting_constructor =  //
    !std::is_same<std::decay_t<T>, outcome>::value                   // not my type
    && base::template enable_exception_converting_constructor<T>;

    //! Predicate for the converting constructor from a compatible input to be available.
    template <class T, class U, class V, class W>
    static constexpr bool enable_compatible_conversion =  //
    !std::is_same<outcome<T, U, V, W>, outcome>::value    // not my type
    && base::template enable_compatible_conversion<T, U, V, W>;

    //! Predicate for the inplace construction of value to be available.
    template <class... Args>
    static constexpr bool enable_inplace_value_constructor =  //
    std::is_void<value_type>::value                           //
    || std::is_constructible<value_type, Args...>::value;

    //! Predicate for the inplace construction of error to be available.
    template <class... Args>
    static constexpr bool enable_inplace_error_constructor =  //
    std::is_void<error_type>::value                           //
    || std::is_constructible<error_type, Args...>::value;

    //! Predicate for the inplace construction of exception to be available.
    template <class... Args>
    static constexpr bool enable_inplace_exception_constructor =  //
    std::is_void<exception_type>::value                           //
    || std::is_constructible<exception_type, Args...>::value;

    // Predicate for the implicit converting inplace constructor to be available.
    template <class... Args>
    static constexpr bool enable_inplace_value_error_exception_constructor =  //
    base::template enable_inplace_value_error_exception_constructor<Args...>;
    template <class... Args> using choose_inplace_value_error_exception_constructor = typename base::template choose_inplace_value_error_exception_constructor<Args...>;
  };

public:
  //! Used to disable in place type construction when `value_type` is ambiguous with `error_type` or `payload_exception_type`.
  using value_type_if_enabled = std::conditional_t<std::is_same<value_type, error_type>::value || std::is_same<value_type, payload_exception_type>::value, disable_in_place_value_type, value_type>;
  //! Used to disable in place type construction when `error_type` is ambiguous with `value_type` or `payload_exception_type`.
  using error_type_if_enabled = std::conditional_t<std::is_same<error_type, value_type>::value || std::is_same<error_type, payload_exception_type>::value, disable_in_place_error_type, error_type>;
  //! Used to disable in place type construction when `payload_exception_type` is ambiguous with `value_type` or `error_type`.
  using exception_type_if_enabled = std::conditional_t<std::is_same<exception_type, value_type>::value || std::is_same<exception_type, error_type>::value, disable_in_place_payload_exception_type, exception_type>;

protected:
  detail::devoid<payload_exception_type> _ptr;

public:
  /// \output_section Converting constructors
  /*! Converting constructor to a successful outcome.
  \tparam enable_value_converting_constructor
  \exclude
  \param 1
  \exclude
  \param t The value from which to initialise the `value_type`.

  \effects Initialises the outcome with a `value_type`.
  \requires Type T is constructible to `value_type`, is not constructible to `status_error_type`, is not constructible to `exception_type` and is not `outcome<R, S, P>` and not `in_place_type<>`.
  \throws Any exception the construction of `value_type(T)` might throw.
  */
  OUTCOME_TEMPLATE(class T)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_value_converting_constructor<T>))
  constexpr outcome(T &&t, value_converting_constructor_tag = value_converting_constructor_tag()) noexcept(std::is_nothrow_constructible<value_type, T>::value)
      : base(in_place_type<typename base::_value_type>, std::forward<T>(t))
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_construction(in_place_type<value_type>, this);
  }
#if OUTCOME_ENABLE_POSITIVE_STATUS
  /*! Converting constructor to a successful outcome + status.
\tparam enable_value_status_converting_constructor
\exclude
\param 2
\exclude
\param t The value from which to initialise the `value_type`.
\param u The value from which to initialise the `status_type`.

\effects Initialises the outcome with a `value_type` and an additional `status_type`.
\requires `trait::status_type_is_negative<EC>` must be false; Type `T` is constructible to `value_type`,
is not constructible to `exception_type`, and is not `outcome<R, S, P>` and not `in_place_type<>`;
Type `U` is constructible to `status_type`, is not constructible to `value_type`, and is not constructible to `exception_type`.
\throws Any exception the construction of `value_type(T)` and `status_type(U)` might throw.
*/
  template <class T, class U, typename enable_value_status_converting_constructor = std::enable_if_t<                                 //
                              !std::is_same<std::decay_t<T>, outcome>::value                                                          // not my type
                              && !detail::is_in_place_type_t<std::decay_t<T>>::value                                                  // not in place construction
                              && detail::is_same_or_constructible<value_type, T> && !std::is_constructible<exception_type, T>::value  //
                              && detail::is_same_or_constructible<status_type, U> && !std::is_constructible<exception_type, U>::value>>
  constexpr outcome(T &&t, U &&u, value_status_converting_constructor_tag = value_status_converting_constructor_tag()) noexcept(std::is_nothrow_constructible<value_type, T>::value &&std::is_nothrow_constructible<status_type, U>::value)
      : base(typename base::value_status_construction_tag(), std::forward<T>(t), std::forward<U>(u))
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_construction(in_place_type<std::pair<value_type, status_type>>, this);
  }
  /*! Converting constructor to a successful outcome + status + payload.
  \tparam enable_value_status_payload_converting_constructor
  \exclude
  \param 3
  \exclude
  \param t The value from which to initialise the `value_type`.
  \param u The value from which to initialise the `status_type`.
  \param v The value from which to initialise the `payload_type`.

  \effects Initialises the outcome with a `value_type`, a `status_type` and a `payload_type`.
  \requires `trait::status_type_is_negative<EC>` must be false; Type `T` is constructible to `value_type`, and is not `outcome<R, S, P>`
  and not `in_place_type<>`;
  Type `U` is constructible to `status_type`;
  Type `V` is constructible to `payload_exception_type`.
  \throws Any exception the construction of `value_type(T)`, `status_type(U)` and `payload_exception_type(V)` might throw.
  */
  template <class T, class U, class V, typename enable_value_status_payload_converting_constructor = std::enable_if_t<  //
                                       !std::is_same<std::decay_t<T>, outcome>::value                                   // not my type
                                       && !detail::is_in_place_type_t<std::decay_t<T>>::value                           // not in place construction
                                       && detail::is_same_or_constructible<value_type, T>                               //
                                       && detail::is_same_or_constructible<status_type, U>                              //
                                       && detail::is_same_or_constructible<payload_exception_type, V>                   //
                                       >>
  constexpr outcome(T &&t, U &&u, V &&v,
                    value_status_payload_converting_constructor_tag = value_status_payload_converting_constructor_tag()) noexcept(std::is_nothrow_constructible<value_type, T>::value &&std::is_nothrow_constructible<status_type, U>::value &&std::is_nothrow_constructible<payload_exception_type, V>::value)
      : base(typename base::value_status_construction_tag(), std::forward<T>(t), std::forward<U>(u))
      , _ptr(std::forward<V>(v))
  {
    using namespace hooks;
    this->_state._status |= detail::status_have_payload;
    hook_outcome_construction(in_place_type<std::tuple<value_type, status_type, payload_type>>, this);
  }
  /*! Converting constructor to a successful outcome + payload.
  \tparam enable_value_payload_converting_constructor_tag
  \exclude
  \param 2
  \exclude
  \param t The value from which to initialise the `value_type`.
  \param u The value from which to initialise the `payload_type`.

  \effects Initialises the outcome with a `value_type` and a `payload_type`.
  \requires `trait::is_exception_ptr<P>` must be false; Type `T` is constructible to `value_type`, is not constructible to `status_error_type`,
  and is not `outcome<R, S, P>` and not `in_place_type<>`;
  Type `U` is constructible to `payload_type`, and is not constructible to `status_error_type`.
  \throws Any exception the construction of `value_type(T)` and `payload_type(U)` might throw.
  */
  template <class T, class U, typename enable_value_payload_converting_constructor_tag = std::enable_if_t<                               //
                              !std::is_same<std::decay_t<T>, outcome>::value                                                             // not my type
                              && !detail::is_in_place_type_t<std::decay_t<T>>::value                                                     // not in place construction
                              && detail::is_same_or_constructible<value_type, T> && !std::is_constructible<status_error_type, T>::value  //
                              && detail::is_same_or_constructible<payload_type, U> && !std::is_constructible<status_error_type, U>::value>>
  constexpr outcome(T &&t, U &&u, value_payload_converting_constructor_tag = value_payload_converting_constructor_tag()) noexcept(std::is_nothrow_constructible<value_type, T>::value &&std::is_nothrow_constructible<payload_exception_type, U>::value)
      : base(in_place_type<typename base::value_type>, std::forward<T>(t))
      , _ptr(std::forward<U>(u))
  {
    using namespace hooks;
    hook_outcome_construction(in_place_type<std::pair<value_type, payload_type>>, this);
  }
#endif
  /*! Converting constructor to an errored outcome.
\tparam enable_error_converting_constructor
\exclude
\param 1
\exclude
\param t The value from which to initialise the `error_type`.

\effects Initialises the outcome with a `error_type`.
\requires `trait::status_type_is_negative<EC>` must be true; Type T is constructible to `error_type`,
is not constructible to `value_type`, is not constructible to `payload_exception_type`, and is not `outcome<R, S, P>` and not `in_place_type<>`.
\throws Any exception the construction of `error_type(T)` might throw.
*/
  OUTCOME_TEMPLATE(class T)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_error_converting_constructor<T>))
  constexpr outcome(T &&t, error_converting_constructor_tag = error_converting_constructor_tag()) noexcept(std::is_nothrow_constructible<error_type, T>::value)
      : base(in_place_type<typename base::_error_type>, std::forward<T>(t))
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_construction(in_place_type<error_type>, this);
  }
  /*! Converting constructor to an errored outcome + payload/exception.
  \tparam enable_error_payload_converting_constructor
  \exclude
  \param 2
  \exclude
  \param t The value from which to initialise the `error_type`.
  \param u The value from which to initialise the `payload_exception_type`.

  \effects Initialises the outcome with a `error_type` and a `payload_exception_type`.
  \requires `trait::status_type_is_negative<EC>` must be true; Type T is constructible to `error_type`,
  is not constructible to `value_type`, and is not `outcome<R, S, P>` and not `in_place_type<>`;
  Type `U` is constructible to `payload_exception_type`, is not constructible to `value_type`.
  \throws Any exception the construction of `error_type(T)` and `payload_exception_type(U)` might throw.
  */
  OUTCOME_TEMPLATE(class T, class U)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_error_payload_converting_constructor<T, U>))
  constexpr outcome(T &&t, U &&u, error_payload_converting_constructor_tag = error_payload_converting_constructor_tag()) noexcept(std::is_nothrow_constructible<error_type, T>::value &&std::is_nothrow_constructible<payload_exception_type, U>::value)
      : base(in_place_type<typename base::_error_type>, std::forward<T>(t))
      , _ptr(std::forward<U>(u))
  {
    using namespace hooks;
    this->_state._status |= trait::is_exception_ptr<payload_exception_type>::value ? detail::status_have_exception : detail::status_have_payload;
    hook_outcome_construction(in_place_type<std::pair<error_type, payload_exception_type>>, this);
  }
  /*! Special error condition converting constructor to an errored outcome.
  \tparam enable_error_condition_converting_constructor
  \exclude
  \param 1
  \exclude
  \param t The error condition from which to initialise the `error_type`.

  \effects Initialises the outcome with a `error_type` constructed via `make_error_code(t)`.
  \requires `trait::status_type_is_negative<EC>` must be true; `std::is_error_condition_enum<ErrorCondEnum>` must be true,
  `ErrorCondEnum` is not constructible to `value_type`, `error_type` nor `payload_exception_type`, and is not `outcome<R, S, P>` and not `in_place_type<>`;
  Finally, the expression `error_type(make_error_code(ErrorCondEnum()))` must be valid.
  \throws Any exception the construction of `error_type(make_error_code(t))` might throw.
  */
  OUTCOME_TEMPLATE(class ErrorCondEnum)
  OUTCOME_TREQUIRES(OUTCOME_TEXPR(error_type(make_error_code(ErrorCondEnum()))),  //
                    OUTCOME_TPRED(predicate::template enable_error_condition_converting_constructor<ErrorCondEnum>))
  constexpr outcome(ErrorCondEnum &&t, error_condition_converting_constructor_tag = error_condition_converting_constructor_tag()) noexcept(noexcept(error_type(make_error_code(std::forward<ErrorCondEnum>(t)))))
      : base(in_place_type<typename base::_error_type>, make_error_code(t))
  {
    using namespace hooks;
    hook_outcome_construction(in_place_type<error_type>, this);
  }
  /*! Converting constructor to an excepted outcome.
  \tparam enable_exception_converting_constructor
  \exclude
  \param 1
  \exclude
  \param t The value from which to initialise the `exception_type`.

  \effects Initialises the outcome with a `exception_type`.
  \requires `trait::is_exception_ptr<P>` must be true; Type T is constructible to `exception_type`,
  is not constructible to `value_type`, is not constructible to `status_error_type`, and is not `outcome<R, S, P>` and not `in_place_type<>`.
  \throws Any exception the construction of `exception_type(T)` might throw.
  */
  OUTCOME_TEMPLATE(class T)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_exception_converting_constructor<T>))
  constexpr outcome(T &&t, exception_converting_constructor_tag = exception_converting_constructor_tag()) noexcept(std::is_nothrow_constructible<exception_type, T>::value)
      : base()
      , _ptr(std::forward<T>(t))
  {
    using namespace hooks;
    this->_state._status |= detail::status_have_exception;
    hook_outcome_construction(in_place_type<exception_type>, this);
  }

  /*! Explicit converting copy constructor from a compatible outcome type.
  \tparam 4
  \exclude
  \param o The compatible outcome.

  \effects Initialises the outcome with a copy of the compatible outcome.
  \requires Both outcome's `value_type`, `error_type`, `status_type`, `payload_type` and `exception_type` need to be constructible, or the source `void`.
  \throws Any exception the construction of `value_type(T)`, `status_error_type(U)` or `payload_exception_type(V)` might throw.
  */
  OUTCOME_TEMPLATE(class T, class U, class V, class W)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_compatible_conversion<T, U, V, W>))
  constexpr explicit outcome(const outcome<T, U, V, W> &o) noexcept(std::is_nothrow_constructible<value_type, T>::value &&std::is_nothrow_constructible<status_error_type, U>::value &&std::is_nothrow_constructible<payload_exception_type, V>::value)
      : base(typename base::compatible_conversion_tag(), o)
      , _ptr(o._ptr)
  {
    using namespace hooks;
    hook_outcome_copy_construction(in_place_type<decltype(o)>, this);
  }
  /*! Explicit converting move constructor from a compatible outcome type.
  \tparam 4
  \exclude
  \param o The compatible outcome.

  \effects Initialises the outcome with a move of the compatible outcome.
  \requires Both outcome's `value_type`, `error_type`, `status_type`, `payload_type` and `exception_type` need to be constructible, or the source `void`.
  \throws Any exception the construction of `value_type(T)`, `status_error_type(U)` or `payload_exception_type(V)` might throw.
  */
  OUTCOME_TEMPLATE(class T, class U, class V, class W)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_compatible_conversion<T, U, V, W>))
  constexpr explicit outcome(outcome<T, U, V, W> &&o) noexcept(std::is_nothrow_constructible<value_type, T>::value &&std::is_nothrow_constructible<status_error_type, U>::value &&std::is_nothrow_constructible<payload_exception_type, V>::value)
      : base(typename base::compatible_conversion_tag(), std::move(o))
      , _ptr(std::move(o._ptr))
  {
    using namespace hooks;
    hook_outcome_move_construction(in_place_type<decltype(o)>, this);
  }
  /*! Explicit converting copy constructor from a compatible result type.
  \tparam 3
  \exclude
  \param o The compatible result.

  \effects Initialises the outcome with a copy of the compatible result.
  \requires Both outcome's `value_type`, `error_type` and `status_type` need to be constructible, or the source `void`.
  \throws Any exception the construction of `value_type(T)`, `status_error_type(U)` or `payload_exception_type()` might throw.
  */
  OUTCOME_TEMPLATE(class T, class U, class V)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(detail::result_predicates<value_type, status_error_type, error_type>::template enable_compatible_conversion<T, U, V>))
  constexpr explicit outcome(const result<T, U, V> &o) noexcept(std::is_nothrow_constructible<value_type, T>::value &&std::is_nothrow_constructible<status_error_type, U>::value &&std::is_nothrow_constructible<payload_exception_type>::value)
      : base(typename base::compatible_conversion_tag(), o)
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_copy_construction(in_place_type<decltype(o)>, this);
  }
  /*! Explicit converting move constructor from a compatible result type.
  \tparam 3
  \exclude
  \param o The compatible result.

  \effects Initialises the outcome with a move of the compatible result.
  \requires Both outcome's `value_type`, `error_type` and `status_type` need to be constructible, or the source `void`.
  \throws Any exception the construction of `value_type(T)`, `status_error_type(U)` or `payload_exception_type()` might throw.
  */
  OUTCOME_TEMPLATE(class T, class U, class V)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(detail::result_predicates<value_type, status_error_type, error_type>::template enable_compatible_conversion<T, U, V>))
  constexpr explicit outcome(result<T, U, V> &&o) noexcept(std::is_nothrow_constructible<value_type, T>::value &&std::is_nothrow_constructible<status_error_type, U>::value &&std::is_nothrow_constructible<payload_exception_type>::value)
      : base(typename base::compatible_conversion_tag(), std::move(o))
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_move_construction(in_place_type<decltype(o)>, this);
  }


  /// \output_section In place constructors
  /*! Inplace constructor to a successful value.
  \tparam 1
  \exclude
  \param _ Tag type to indicate we are doing in place construction of `value_type`.
  \param args Arguments with which to in place construct.

  \effects Initialises the outcome with a `value_type`.
  \requires `value_type` is void or `Args...` are constructible to `value_type`.
  \throws Any exception the construction of `value_type(Args...)` might throw.
  */
  OUTCOME_TEMPLATE(class... Args)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_inplace_value_constructor<Args...>))
  constexpr explicit outcome(in_place_type_t<value_type_if_enabled> _, Args &&... args) noexcept(std::is_nothrow_constructible<value_type, Args...>::value)
      : base(_, std::forward<Args>(args)...)
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_in_place_construction(in_place_type<value_type>, this);
  }
  /*! Inplace constructor to a successful value.
  \tparam 2
  \exclude
  \param _ Tag type to indicate we are doing in place construction of `value_type`.
  \param il An initializer list with which to in place construct.
  \param args Arguments with which to in place construct.

  \effects Initialises the outcome with a `value_type`.
  \requires The initializer list + `Args...` are constructible to `value_type`.
  \throws Any exception the construction of `value_type(il, Args...)` might throw.
  */
  OUTCOME_TEMPLATE(class U, class... Args)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_inplace_value_constructor<std::initializer_list<U>, Args...>))
  constexpr explicit outcome(in_place_type_t<value_type_if_enabled> _, std::initializer_list<U> il, Args &&... args) noexcept(std::is_nothrow_constructible<value_type, std::initializer_list<U>, Args...>::value)
      : base(_, il, std::forward<Args>(args)...)
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_in_place_construction(in_place_type<value_type>, this);
  }
  /*! Inplace constructor to an unsuccessful error.
  \tparam 1
  \exclude
  \param _ Tag type to indicate we are doing in place construction of `error_type`.
  \param args Arguments with which to in place construct.

  \effects Initialises the outcome with a `error_type`.
  \requires `trait::status_type_is_negative<EC>` must be true; `error_type` is void or `Args...` are constructible to `error_type`.
  \throws Any exception the construction of `error_type(Args...)` might throw.
  */
  OUTCOME_TEMPLATE(class... Args)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_inplace_error_constructor<Args...>))
  constexpr explicit outcome(in_place_type_t<error_type_if_enabled> _, Args &&... args) noexcept(std::is_nothrow_constructible<error_type, Args...>::value)
      : base(_, std::forward<Args>(args)...)
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_in_place_construction(in_place_type<error_type>, this);
  }
  /*! Inplace constructor to an unsuccessful error.
  \tparam 2
  \exclude
  \param _ Tag type to indicate we are doing in place construction of `error_type`.
  \param il An initializer list with which to in place construct.
  \param args Arguments with which to in place construct.

  \effects Initialises the outcome with a `error_type`.
  \requires `trait::status_type_is_negative<EC>` must be true; The initializer list + `Args...` are constructible to `error_type`.
  \throws Any exception the construction of `error_type(il, Args...)` might throw.
  */
  OUTCOME_TEMPLATE(class U, class... Args)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_inplace_error_constructor<std::initializer_list<U>, Args...>))
  constexpr explicit outcome(in_place_type_t<error_type_if_enabled> _, std::initializer_list<U> il, Args &&... args) noexcept(std::is_nothrow_constructible<error_type, std::initializer_list<U>, Args...>::value)
      : base(_, il, std::forward<Args>(args)...)
      , _ptr()
  {
    using namespace hooks;
    hook_outcome_in_place_construction(in_place_type<error_type>, this);
  }
  /*! Inplace constructor to an unsuccessful exception.
  \tparam 1
  \exclude
  \param _ Tag type to indicate we are doing in place construction of `exception_type`.
  \param args Arguments with which to in place construct.

  \effects Initialises the outcome with an `exception_type`.
  \requires `exception_type` is void or `Args...` are constructible to `exception_type`.
  \throws Any exception the construction of `exception_type(Args...)` might throw.
  */
  OUTCOME_TEMPLATE(class... Args)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_inplace_exception_constructor<Args...>))
  constexpr explicit outcome(in_place_type_t<exception_type_if_enabled>, Args &&... args) noexcept(std::is_nothrow_constructible<exception_type, Args...>::value)
      : base()
      , _ptr(std::forward<Args>(args)...)
  {
    using namespace hooks;
    this->_state._status |= detail::status_have_exception;
    hook_outcome_in_place_construction(in_place_type<exception_type>, this);
  }
  /*! Inplace constructor to an unsuccessful exception.
  \tparam 2
  \exclude
  \param _ Tag type to indicate we are doing in place construction of `exception_type`.
  \param il An initializer list with which to in place construct.
  \param args Arguments with which to in place construct.

  \effects Initialises the outcome with an `exception_type`.
  \requires The initializer list + `Args...` are constructible to `exception_type`.
  \throws Any exception the construction of `exception_type(il, Args...)` might throw.
  */
  OUTCOME_TEMPLATE(class U, class... Args)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_inplace_exception_constructor<std::initializer_list<U>, Args...>))
  constexpr explicit outcome(in_place_type_t<exception_type_if_enabled>, std::initializer_list<U> il, Args &&... args) noexcept(std::is_nothrow_constructible<exception_type, std::initializer_list<U>, Args...>::value)
      : base()
      , _ptr(il, std::forward<Args>(args)...)
  {
    using namespace hooks;
    this->_state._status |= detail::status_have_exception;
    hook_outcome_in_place_construction(in_place_type<exception_type>, this);
  }
  /*! Implicit inplace constructor to successful value, or unsuccessful error, or unsuccessful exception.
  \tparam 3
  \exclude
  \param args Arguments with which to in place construct.

  \effects Calls the appropriate `in_place_type_t<...>` constructor depending on constructibility of args.
  \requires That the args can construct exactly one of `value_type` or `error_type` or `exception_type`.
  \throws Any exception the `in_place_type_t<...>` constructor might throw.
  */
  OUTCOME_TEMPLATE(class A1, class A2, class... Args)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_inplace_value_error_exception_constructor<A1, A2, Args...>))
  constexpr outcome(A1 &&a1, A2 &&a2, Args &&... args) noexcept(noexcept(typename predicate::template choose_inplace_value_error_exception_constructor<A1, A2, Args...>(std::declval<A1>(), std::declval<A2>(), std::declval<Args>()...)))
      : outcome(in_place_type<typename predicate::template choose_inplace_value_error_exception_constructor<A1, A2, Args...>>, std::forward<A1>(a1), std::forward<A2>(a2), std::forward<Args>(args)...)
  {
  }

  /// \output_section Tagged constructors
  /*! Implicit tagged constructor of a successful outcome.
  \param o The compatible success type sugar.

  \effects Initialises the outcome with a default constructed success type.
  \requires  `value_type` to be default constructible, or `void`.
  \throws Any exception the construction of `value_type()` might throw.
  */
  constexpr outcome(const success_type<void> &o) noexcept(std::is_nothrow_default_constructible<value_type>::value)
      : base(in_place_type<typename base::_value_type>)
  {
    using namespace hooks;
    hook_outcome_copy_construction(in_place_type<decltype(o)>, this);
  }
  /*! Implicit tagged constructor of a successful outcome.
  \tparam 1
  \exclude
  \param o The compatible success type sugar.

  \effects Initialises the outcome with a copy of the value in the type sugar.
  \requires Both outcome and success' `value_type` need to be constructible. The source cannot be `void`.
  \throws Any exception the construction of `value_type(T)` might throw.
  */
  OUTCOME_TEMPLATE(class T)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(!std::is_void<T>::value && predicate::template enable_compatible_conversion<T, void, void, void>))
  constexpr outcome(const success_type<T> &o) noexcept(std::is_nothrow_constructible<value_type, T>::value)
      : base(in_place_type<typename base::_value_type>, detail::extract_value_from_success<value_type>(o))
  {
    using namespace hooks;
    hook_outcome_copy_construction(in_place_type<decltype(o)>, this);
  }
  /*! Implicit tagged constructor of a successful outcome.
  \tparam 1
  \exclude
  \param o The compatible success type sugar.

  \effects Initialises the outcome with a move of the value in the type sugar.
  \requires Both outcome and success' `value_type` need to be constructible. The source cannot be `void`.
  \throws Any exception the construction of `value_type(T)` might throw.
  */
  OUTCOME_TEMPLATE(class T)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(!std::is_void<T>::value && predicate::template enable_compatible_conversion<T, void, void, void>))
  constexpr outcome(success_type<T> &&o) noexcept(std::is_nothrow_constructible<value_type, T>::value)
      : base(in_place_type<typename base::_value_type>, std::move(detail::extract_value_from_success<value_type>(std::move(o))))
  {
    using namespace hooks;
    hook_outcome_move_construction(in_place_type<decltype(o)>, this);
  }
  /*! Implicit tagged constructor of a failure outcome.
  \tparam 2
  \exclude
  \param o The compatible failure type sugar.

  \effects Initialises the outcome with a copy of the error and/or exception in the type sugar.
  \requires Both outcome and failure's `error_type` and `exception_type` need to be constructible, or the source can be `void`.
  \throws Any exception the construction of `error_type(T)` and/or `exception_type(U)` might throw.
  */
  OUTCOME_TEMPLATE(class T, class U)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_compatible_conversion<void, T, U, void>))
  constexpr outcome(const failure_type<T, U> &o) noexcept(std::is_nothrow_constructible<error_type, T>::value &&std::is_nothrow_constructible<exception_type, U>::value)
      : base(in_place_type<typename base::_error_type>, detail::extract_error_from_failure<error_type>(o))
      , _ptr(detail::extract_exception_payload_from_failure<exception_type>(o))
  {
    using namespace hooks;
    hook_outcome_copy_construction(in_place_type<decltype(o)>, this);
  }
  /*! Implicit tagged constructor of a failure outcome.
  \tparam 2
  \exclude
  \param o The compatible failure type sugar.

  \effects Initialises the outcome with a copy of the error and/or exception in the type sugar.
  \requires Both outcome and failure's `error_type` and `exception_type` need to be constructible, or the source can be `void`.
  \throws Any exception the construction of `error_type(T)` and/or `exception_type(U)` might throw.
  */
  OUTCOME_TEMPLATE(class T, class U)
  OUTCOME_TREQUIRES(OUTCOME_TPRED(predicate::template enable_compatible_conversion<void, T, U, void>))
  constexpr outcome(failure_type<T, U> &&o) noexcept(std::is_nothrow_constructible<error_type, T>::value &&std::is_nothrow_constructible<exception_type, U>::value)
      : base(in_place_type<typename base::_error_type>, std::move(detail::extract_error_from_failure<error_type>(std::move(o))))
      , _ptr(std::move(detail::extract_exception_payload_from_failure<decltype(_ptr)>(std::move(o))))
  {
    using namespace hooks;
    hook_outcome_move_construction(in_place_type<decltype(o)>, this);
  }

  /// \output_section Comparison operators
  using base::operator==;
  using base::operator!=;
  /*! True if equal to the other outcome.
  \param o The other outcome to compare to.

  \effects If a valid expression to do so, calls the `operator==` operation on each
  of the three stored items returning true if both are true. Otherwise returns false.
  \throws Any exception the individual `operator==` operations might throw.
  */
  template <class T, class U, class V, class W>
  constexpr bool operator==(const outcome<T, U, V, W> &o) const noexcept(                                        //
  noexcept(detail::safe_compare_equal(std::declval<detail::devoid<R>>(), std::declval<detail::devoid<T>>()))     //
  && noexcept(detail::safe_compare_equal(std::declval<detail::devoid<S>>(), std::declval<detail::devoid<U>>()))  //
  && noexcept(detail::safe_compare_equal(std::declval<detail::devoid<P>>(), std::declval<detail::devoid<V>>())))
  {
    if(this->_state._status == o._state._status)
    {
      if(!base::operator==(o))
      {
        return false;
      }
      if((this->_state._status & detail::status_have_exception) || (this->_state._status & detail::status_have_payload))
      {
        return detail::safe_compare_equal(this->_ptr, o._ptr);
      }
    }
    return true;
  }
  /*! True if equal to the failure type sugar.
  \param o The failure type sugar to compare to.

  \effects If a valid expression to do so, calls the `operator==` operation on the failure items returning true if equal. Otherwise returns false.
  \throws Any exception the `operator==` operations might throw.
  */
  template <class T, class U>
  constexpr bool operator==(const failure_type<T, U, false> &o) const noexcept(                               //
  noexcept(detail::safe_compare_equal(std::declval<detail::devoid<S>>(), std::declval<detail::devoid<T>>()))  //
  && noexcept(detail::safe_compare_equal(std::declval<detail::devoid<P>>(), std::declval<detail::devoid<U>>())))
  {
    if(!(this->_state._status & detail::status_have_payload))
      return false;
    if(this->_state._status & detail::status_have_error)
    {
      if(!detail::safe_compare_equal(this->_error, o.error))
        return false;
    }
    if((this->_state._status & detail::status_have_exception) || (this->_state._status & detail::status_have_payload))
    {
      return detail::safe_compare_equal(this->_ptr, o.payload);
    }
    return true;
  }
  /*! True if equal to the failure type sugar.
  \param o The failure type sugar to compare to.

  \effects If a valid expression to do so, calls the `operator==` operation on the failure items returning true if equal. Otherwise returns false.
  \throws Any exception the `operator==` operations might throw.
  */
  template <class T, class U>
  constexpr bool operator==(const failure_type<T, U, true> &o) const noexcept(                                //
  noexcept(detail::safe_compare_equal(std::declval<detail::devoid<S>>(), std::declval<detail::devoid<T>>()))  //
  && noexcept(detail::safe_compare_equal(std::declval<detail::devoid<P>>(), std::declval<detail::devoid<U>>())))
  {
    if(!(this->_state._status & detail::status_have_exception))
      return false;
    if(this->_state._status & detail::status_have_error)
    {
      if(!detail::safe_compare_equal(this->_error, o.error))
        return false;
    }
    if((this->_state._status & detail::status_have_exception) || (this->_state._status & detail::status_have_payload))
    {
      return detail::safe_compare_equal(this->_ptr, o.exception);
    }
    return true;
  }
  /*! True if not equal to the other outcome.
  \param o The other outcome to compare to.

  \effects If a valid expression to do so, calls the `operator!=` operation on each
  of the three stored items, returning true if any are not equal. Otherwise returns true.
  \throws Any exception the individual `operator!=` operations might throw.
  */
  template <class T, class U, class V, class W>
  constexpr bool operator!=(const outcome<T, U, V, W> &o) const noexcept(                                           //
  noexcept(detail::safe_compare_notequal(std::declval<detail::devoid<R>>(), std::declval<detail::devoid<T>>()))     //
  && noexcept(detail::safe_compare_notequal(std::declval<detail::devoid<S>>(), std::declval<detail::devoid<U>>()))  //
  && noexcept(detail::safe_compare_notequal(std::declval<detail::devoid<P>>(), std::declval<detail::devoid<V>>())))
  {
    if(this->_state._status != o._state._status)
    {
      return true;
    }
    if(base::operator!=(o))
    {
      return true;
    }
    if((this->_state._status & detail::status_have_exception) || (this->_state._status & detail::status_have_payload))
    {
      return detail::safe_compare_notequal(this->_ptr, o._ptr);
    }
    return false;
  }
  /*! True if not equal to the failure type sugar.
  \param o The failure type sugar to compare to.

  \effects If a valid expression to do so, calls the `operator!=` operation on the failure items returning true if not equal. Otherwise returns true.
  \throws Any exception the `operator!=` operations might throw.
  */
  template <class T, class U>
  constexpr bool operator!=(const failure_type<T, U, false> &o) const noexcept(                                  //
  noexcept(detail::safe_compare_notequal(std::declval<detail::devoid<S>>(), std::declval<detail::devoid<T>>()))  //
  && noexcept(detail::safe_compare_notequal(std::declval<detail::devoid<P>>(), std::declval<detail::devoid<U>>())))
  {
    if(!(this->_state._status & detail::status_have_payload))
    {
      return true;
    }
    if(this->_state._status & detail::status_have_error)
    {
      if(detail::safe_compare_notequal(this->_error, o.error))
      {
        return true;
      }
    }
    if((this->_state._status & detail::status_have_exception) || (this->_state._status & detail::status_have_payload))
    {
      return detail::safe_compare_notequal(this->_ptr, o.payload);
    }
    return false;
  }
  /*! True if not equal to the failure type sugar.
  \param o The failure type sugar to compare to.

  \effects If a valid expression to do so, calls the `operator!=` operation on the failure items returning true if not equal. Otherwise returns false.
  \throws Any exception the `operator!=` operations might throw.
  */
  template <class T, class U>
  constexpr bool operator!=(const failure_type<T, U, true> &o) const noexcept(                                   //
  noexcept(detail::safe_compare_notequal(std::declval<detail::devoid<S>>(), std::declval<detail::devoid<T>>()))  //
  && noexcept(detail::safe_compare_notequal(std::declval<detail::devoid<P>>(), std::declval<detail::devoid<U>>())))
  {
    if(!(this->_state._status & detail::status_have_exception))
    {
      return true;
    }
    if(this->_state._status & detail::status_have_error)
    {
      if(detail::safe_compare_notequal(this->_error, o.error))
      {
        return true;
      }
    }
    if((this->_state._status & detail::status_have_exception) || (this->_state._status & detail::status_have_payload))
    {
      return detail::safe_compare_notequal(this->_ptr, o.exception);
    }
    return false;
  }

  /// \output_section Swap
  /*! Swaps this result with another result
  \effects Any `R` and/or `S` is swapped along with the metadata tracking them.
  */
  void swap(outcome &o) noexcept(detail::is_nothrow_swappable<value_type>::value           //
                                 &&detail::is_nothrow_swappable<status_error_type>::value  //
                                 &&detail::is_nothrow_swappable<payload_exception_type>::value)
  {
    using std::swap;
#ifdef __cpp_exceptions
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4297)  // use of throw in noexcept function
#endif
    this->_state.swap(o._state);
    try
    {
      swap(this->_error, o._error);
      try
      {
        swap(this->_ptr, o._ptr);
      }
      catch(...)
      {
        swap(this->_state, o._state);
        swap(this->_error, o._error);
        throw;
      }
    }
    catch(...)
    {
      swap(this->_state, o._state);
      throw;
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#else
    swap(this->_state, o._state);
    swap(this->_error, o._error);
    swap(this->_ptr, o._ptr);
#endif
  }

  /// \output_section Converters
  /*! Returns this outcome as a `failure_type` with any errored and/or excepted state copied.
  \requires This outcome to have a failed state, else whatever `assume_error()` would do.
  */
  auto as_failure() const & { return failure(this->assume_error(), _ptr); }
  /*! Returns this outcome as a `failure_type` with any errored and/or excepted state moved.
  \requires This outcome to have a failed state, else whatever `assume_error()` would do.
  */
  auto as_failure() && { return failure(std::move(this->assume_error()), std::move(_ptr)); }
};

/*! True if the result is equal to the outcome
\tparam 7
\exclude
\param a The result to compare.
\param b The outcome to compare.

\effects Calls `b == a`.
\requires That the expression `b == a` is a valid expression.
\throws Any exception that `b == a` might throw.
*/
OUTCOME_TEMPLATE(class T, class U, class V,  //
                 class R, class S, class P, class N)
OUTCOME_TREQUIRES(OUTCOME_TEXPR(std::declval<outcome<R, S, P, N>>() == std::declval<result<T, U, V>>()))
constexpr inline bool operator==(const result<T, U, V> &a, const outcome<R, S, P, N> &b) noexcept(  //
noexcept(std::declval<outcome<R, S, P, N>>() == std::declval<result<T, U, V>>()))
{
  return b == a;
}
/*! True if the result is not equal to the outcome
\tparam 7
\exclude
\param a The result to compare.
\param b The outcome to compare.

\effects Calls `b != a`.
\requires That the expression `b != a` is a valid expression.
\throws Any exception that `b != a` might throw.
*/
OUTCOME_TEMPLATE(class T, class U, class V,  //
                 class R, class S, class P, class N)
OUTCOME_TREQUIRES(OUTCOME_TEXPR(std::declval<outcome<R, S, P, N>>() != std::declval<result<T, U, V>>()))
constexpr inline bool operator!=(const result<T, U, V> &a, const outcome<R, S, P, N> &b) noexcept(  //
noexcept(std::declval<outcome<R, S, P, N>>() != std::declval<result<T, U, V>>()))
{
  return b != a;
}
/*! Specialise swap for outcome.
\effects Calls `a.swap(b)`.
*/
template <class R, class S, class P, class N> inline void swap(outcome<R, S, P, N> &a, outcome<R, S, P, N> &b) noexcept(noexcept(a.swap(b)))
{
  a.swap(b);
}

namespace hooks
{
  /*! Used to set/override a payload/exception during a construction hook implementation.
  \param o The outcome you wish to change.
  \param v Payload/Exception to be set.
  \effects Sets the payload/exception of the outcome to the given value.
  */
  template <class R, class S, class P, class NoValuePolicy, class U> constexpr inline void override_outcome_payload_exception(outcome<R, S, P, NoValuePolicy> *o, U &&v) noexcept
  {
    o->_ptr = std::forward<U>(v);
    if(trait::is_exception_ptr<P>::value)
      o->_state._status |= detail::status_have_exception;
    else
      o->_state._status |= detail::status_have_payload;
  }
}

namespace policy
{
#ifdef __cpp_exceptions
  /*! Policy interpreting S as a type implementing the `std::error_code` contract, E as
  a type implementing the `std::exception_ptr` contract, and any wide attempt to access the
  successful state throws the `exception_ptr` if available, then the `error_code` wrapped
  into a `std::system_error`.
  \module Error code interpretation policy
  */
  template <class R, class S, class P> struct error_code_throw_as_system_error_exception_rethrow
  {
    static_assert(std::is_base_of<std::error_code, S>::value, "error_type must be a base of std::error_code to be used with this policy");
    static_assert(std::is_base_of<std::exception_ptr, P>::value, "exception_type must be a base of std::exception_ptr to be used with this policy");
    /*! Performs a narrow check of state, used in the assume_value() functions.
    \effects None.
    */
    template <class Impl> static constexpr void narrow_value_check(Impl *self) noexcept
    {
      (void) self;
#if defined(__GNUC__) || defined(__clang__)
      if((self->_state._status & detail::status_have_value) == 0)
        __builtin_unreachable();
#endif
    }
    /*! Performs a narrow check of state, used in the assume_error() functions.
    \effects None.
    */
    template <class Impl> static constexpr void narrow_error_check(Impl *self) noexcept
    {
      (void) self;
#if defined(__GNUC__) || defined(__clang__)
      if((self->_state._status & detail::status_have_error) == 0)
        __builtin_unreachable();
#endif
    }
#if OUTCOME_ENABLE_POSITIVE_STATUS
    /*! Performs a narrow check of state, used in the assume_status() functions
\effects None.
*/
    template <class Impl> static constexpr void narrow_status_check(Impl *self) noexcept
    {
      (void) self;
#if defined(__GNUC__) || defined(__clang__)
      if((self->_state._status & detail::status_have_status) == 0)
        __builtin_unreachable();
#endif
    }
#endif
    /*! Performs a narrow check of state, used in the assume_payload() functions.
    \effects None.
    */
    template <class Impl> static constexpr void narrow_payload_check(Impl *self) noexcept
    {
      (void) self;
#if defined(__GNUC__) || defined(__clang__)
      if((self->_state._status & detail::status_have_payload) == 0)
        __builtin_unreachable();
#endif
    }
    /*! Performs a narrow check of state, used in the assume_exception() functions.
    \effects None.
    */
    template <class Impl> static constexpr void narrow_exception_check(Impl *self) noexcept
    {
      (void) self;
#if defined(__GNUC__) || defined(__clang__)
      if((self->_state._status & detail::status_have_exception) == 0)
        __builtin_unreachable();
#endif
    }
    /*! Performs a wide check of state, used in the value() functions.
    \effects If outcome does not have a value, if it has an exception it rethrows it via `std::rethrow_exception()`,
    if has an error it throws a `std::system_error(error())`, else it throws `bad_outcome_access`.
    */
    template <class Impl> static constexpr void wide_value_check(Impl *self)
    {
      if((self->_state._status & detail::status_have_value) == 0)
      {
        if((self->_state._status & detail::status_have_exception) != 0)
        {
          auto *_self = static_cast<const outcome<R, S, P, error_code_throw_as_system_error_exception_rethrow> *>(self);
          std::rethrow_exception(_self->_ptr);
        }
        if((self->_state._status & detail::status_have_error) != 0)
        {
          throw std::system_error(self->_error);
        }
        throw bad_outcome_access("no value");
      }
    }
    /*! Performs a wide check of state, used in the error() functions
    \effects If outcome does not have an error, it throws `bad_outcome_access`.
    */
    template <class Impl> static constexpr void wide_error_check(Impl *self)
    {
      if((self->_state._status & detail::status_have_error) == 0)
      {
        throw bad_outcome_access("no error");
      }
    }
#if OUTCOME_ENABLE_POSITIVE_STATUS
    /*! Performs a wide check of state, used in the status() functions
\effects If outcome does not have an status, it throws `bad_outcome_access`.
*/
    template <class Impl> static constexpr void wide_status_check(Impl *self)
    {
      if((self->_state._status & detail::status_have_status) == 0)
      {
        throw bad_outcome_access("no status");
      }
    }
#endif
    /*! Performs a wide check of state, used in the payload() functions
    \effects If outcome does not have a payload, it throws `bad_outcome_access`.
    */
    template <class Impl> static constexpr void wide_payload_check(Impl *self)
    {
      if((self->_state._status & detail::status_have_payload) == 0)
      {
        throw bad_outcome_access("no payload");
      }
    }
    /*! Performs a wide check of state, used in the exception() functions
    \effects If outcome does not have an exception, it throws `bad_outcome_access`.
    */
    template <class Impl> static constexpr void wide_exception_check(Impl *self)
    {
      if((self->_state._status & detail::status_have_exception) == 0)
      {
        throw bad_outcome_access("no exception");
      }
    }
  };
#endif
}
namespace impl
{
  template <class Base, class R, class S, class P, class NoValuePolicy> inline constexpr typename outcome_payload_observers<Base, R, S, P, NoValuePolicy>::payload_type &outcome_payload_observers<Base, R, S, P, NoValuePolicy>::assume_payload() & noexcept
  {
    auto *self = static_cast<outcome<R, S, P, NoValuePolicy> *>(this);
    NoValuePolicy::narrow_payload_check(this);
    return self->_ptr;
  }
  template <class Base, class R, class S, class P, class NoValuePolicy> inline constexpr const typename outcome_payload_observers<Base, R, S, P, NoValuePolicy>::payload_type &outcome_payload_observers<Base, R, S, P, NoValuePolicy>::assume_payload() const &noexcept
  {
    auto *self = static_cast<const outcome<R, S, P, NoValuePolicy> *>(this);
    NoValuePolicy::narrow_payload_check(this);
    return self->_ptr;
  }
  template <class Base, class R, class S, class P, class NoValuePolicy> inline constexpr typename outcome_payload_observers<Base, R, S, P, NoValuePolicy>::payload_type &&outcome_payload_observers<Base, R, S, P, NoValuePolicy>::assume_payload() && noexcept
  {
    auto *self = static_cast<outcome<R, S, P, NoValuePolicy> *>(this);
    NoValuePolicy::narrow_payload_check(this);
    return std::move(self->_ptr);
  }
  template <class Base, class R, class S, class P, class NoValuePolicy> inline constexpr const typename outcome_payload_observers<Base, R, S, P, NoValuePolicy>::payload_type &&outcome_payload_observers<Base, R, S, P, NoValuePolicy>::assume_payload() const &&noexcept
  {
    auto *self = static_cast<const outcome<R, S, P, NoValuePolicy> *>(this);
    NoValuePolicy::narrow_payload_check(this);
    return std::move(self->_ptr);
  }

  template <class Base, class R, class S, class P, class NoValuePolicy> inline constexpr typename outcome_payload_observers<Base, R, S, P, NoValuePolicy>::payload_type &outcome_payload_observers<Base, R, S, P, NoValuePolicy>::payload() &
  {
    auto *self = static_cast<outcome<R, S, P, NoValuePolicy> *>(this);
    NoValuePolicy::wide_payload_check(this);
    return self->_ptr;
  }
  template <class Base, class R, class S, class P, class NoValuePolicy> inline constexpr const typename outcome_payload_observers<Base, R, S, P, NoValuePolicy>::payload_type &outcome_payload_observers<Base, R, S, P, NoValuePolicy>::payload() const &
  {
    auto *self = static_cast<const outcome<R, S, P, NoValuePolicy> *>(this);
    NoValuePolicy::wide_payload_check(this);
    return self->_ptr;
  }
  template <class Base, class R, class S, class P, class NoValuePolicy> inline constexpr typename outcome_payload_observers<Base, R, S, P, NoValuePolicy>::payload_type &&outcome_payload_observers<Base, R, S, P, NoValuePolicy>::payload() &&
  {
    auto *self = static_cast<outcome<R, S, P, NoValuePolicy> *>(this);
    NoValuePolicy::wide_payload_check(this);
    return std::move(self->_ptr);
  }
  template <class Base, class R, class S, class P, class NoValuePolicy> inline constexpr const typename outcome_payload_observers<Base, R, S, P, NoValuePolicy>::payload_type &&outcome_payload_observers<Base, R, S, P, NoValuePolicy>::payload() const &&
  {
    auto *self = static_cast<const outcome<R, S, P, NoValuePolicy> *>(this);
    NoValuePolicy::wide_payload_check(this);
    return std::move(self->_ptr);
  }

  template <class Base, class R, class S, class P, class NoValuePolicy> inline constexpr typename outcome_exception_observers<Base, R, S, P, NoValuePolicy>::exception_type &outcome_exception_observers<Base, R, S, P, NoValuePolicy>::assume_exception() & noexcept
  {
    auto *self = static_cast<outcome<R, S, P, NoValuePolicy> *>(this);
    NoValuePolicy::narrow_exception_check(this);
    return self->_ptr;
  }
  template <class Base, class R, class S, class P, class NoValuePolicy> inline constexpr const typename outcome_exception_observers<Base, R, S, P, NoValuePolicy>::exception_type &outcome_exception_observers<Base, R, S, P, NoValuePolicy>::assume_exception() const &noexcept
  {
    auto *self = static_cast<const outcome<R, S, P, NoValuePolicy> *>(this);
    NoValuePolicy::narrow_exception_check(this);
    return self->_ptr;
  }
  template <class Base, class R, class S, class P, class NoValuePolicy> inline constexpr typename outcome_exception_observers<Base, R, S, P, NoValuePolicy>::exception_type &&outcome_exception_observers<Base, R, S, P, NoValuePolicy>::assume_exception() && noexcept
  {
    auto *self = static_cast<outcome<R, S, P, NoValuePolicy> *>(this);
    NoValuePolicy::narrow_exception_check(this);
    return std::move(self->_ptr);
  }
  template <class Base, class R, class S, class P, class NoValuePolicy> inline constexpr const typename outcome_exception_observers<Base, R, S, P, NoValuePolicy>::exception_type &&outcome_exception_observers<Base, R, S, P, NoValuePolicy>::assume_exception() const &&noexcept
  {
    auto *self = static_cast<const outcome<R, S, P, NoValuePolicy> *>(this);
    NoValuePolicy::narrow_exception_check(this);
    return std::move(self->_ptr);
  }

  template <class Base, class R, class S, class P, class NoValuePolicy> inline constexpr typename outcome_exception_observers<Base, R, S, P, NoValuePolicy>::exception_type &outcome_exception_observers<Base, R, S, P, NoValuePolicy>::exception() &
  {
    auto *self = static_cast<outcome<R, S, P, NoValuePolicy> *>(this);
    NoValuePolicy::wide_exception_check(this);
    return self->_ptr;
  }
  template <class Base, class R, class S, class P, class NoValuePolicy> inline constexpr const typename outcome_exception_observers<Base, R, S, P, NoValuePolicy>::exception_type &outcome_exception_observers<Base, R, S, P, NoValuePolicy>::exception() const &
  {
    auto *self = static_cast<const outcome<R, S, P, NoValuePolicy> *>(this);
    NoValuePolicy::wide_exception_check(this);
    return self->_ptr;
  }
  template <class Base, class R, class S, class P, class NoValuePolicy> inline constexpr typename outcome_exception_observers<Base, R, S, P, NoValuePolicy>::exception_type &&outcome_exception_observers<Base, R, S, P, NoValuePolicy>::exception() &&
  {
    auto *self = static_cast<outcome<R, S, P, NoValuePolicy> *>(this);
    NoValuePolicy::wide_exception_check(this);
    return std::move(self->_ptr);
  }
  template <class Base, class R, class S, class P, class NoValuePolicy> inline constexpr const typename outcome_exception_observers<Base, R, S, P, NoValuePolicy>::exception_type &&outcome_exception_observers<Base, R, S, P, NoValuePolicy>::exception() const &&
  {
    auto *self = static_cast<const outcome<R, S, P, NoValuePolicy> *>(this);
    NoValuePolicy::wide_exception_check(this);
    return std::move(self->_ptr);
  }
}
OUTCOME_V2_NAMESPACE_END

#endif
