//
// Copyright [2020] <inhzus>
//

#ifndef TOYS_STREAM_TRAITS_H_
#define TOYS_STREAM_TRAITS_H_

#include <functional>
#include <tuple>

template <typename T>
struct remove_func_class {
  using type = T;
};
template <typename R, typename C, typename... Args>
struct remove_func_class<R (C::*)(Args...)>
    : public remove_func_class<R (*)(Args...)> {};
template <typename R, typename C, typename... Args>
struct remove_func_class<R (C::*)(Args...) const>
    : public remove_func_class<R (*)(Args...)> {};
template <typename R, typename C, typename... Args>
struct remove_func_class<R (C::*)(Args...) volatile>
    : public remove_func_class<R (*)(Args...)> {};
template <typename R, typename C, typename... Args>
struct remove_func_class<R (C::*)(Args...) const volatile>
    : public remove_func_class<R (*)(Args...)> {};

template <typename T>
using remove_func_class_t = typename remove_func_class<T>::type;

template <typename T, typename = void>
struct func_traits : func_traits<remove_func_class_t<T>> {};
template <typename R, typename... Args>
struct func_traits<R (*)(Args...)> {
  using result_type = R;
  using args_type = std::tuple<Args...>;
  template <size_t idx>
  using arg_type_at = std::tuple_element_t<idx, args_type>;
  static constexpr size_t arg_count = sizeof...(Args);
};
template <typename T>
struct func_traits<T, decltype(&T::operator(), void())>
    : public func_traits<decltype(&T::operator())> {};

template <typename Func, size_t idx, typename... Args>
struct __func_args_decay_to_impl {};

template <typename Func, size_t idx>
struct __func_args_decay_to_impl<Func, idx> {
  static constexpr bool value = func_traits<Func>::arg_count == idx;
};

template <typename Func, size_t idx, typename Arg, typename... Args>
struct __func_args_decay_to_impl<Func, idx, Arg, Args...> {
  static constexpr bool value =
      std::is_same_v<
          std::decay_t<typename func_traits<Func>::template arg_type_at<idx>>,
          Arg> &&
      __func_args_decay_to_impl<Func, idx + 1, Args...>::value;
};

template <typename Func, typename... Args>
struct func_args_decay_to : public __func_args_decay_to_impl<Func, 0, Args...> {
};

template <typename Func, typename... Args>
inline constexpr bool func_args_decay_to_v =
    func_args_decay_to<Func, Args...>::value;

template <typename Func, typename R>
inline constexpr bool func_return_as_v =
    std::is_same_v<typename func_traits<Func>::result_type, R>;

template <typename C>
using value_type_of = std::decay_t<decltype(*std::declval<C>().begin())>;

template <typename C, typename T = value_type_of<C>>
inline constexpr bool is_container_of =
    std::is_same_v<T, std::decay_t<decltype(*std::declval<C>().end())>>
        &&std::is_convertible_v<decltype(std::declval<C>().size()), size_t>;

#endif  // TOYS_STREAM_TRAITS_H_
