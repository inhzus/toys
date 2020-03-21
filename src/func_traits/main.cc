//
// Copyright [2020] <inhzus>
//
// function traits for lambda, member and raw functions.
// TODO: template functions.

#include <functional>

template <typename T> struct remove_func_class { using type = T; };
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
template <typename R, typename... Args> struct func_traits<R (*)(Args...)> {
  using result_type = R;
  using args_type = std::tuple<Args...>;
  template <size_t idx>
  using arg_type_at = std::tuple_element_t<idx, args_type>;
  static constexpr size_t arg_count = sizeof...(Args);
};
template <typename T>
struct func_traits<T, decltype(&T::operator(), void())>
    : public func_traits<decltype(&T::operator())> {};
// template <typename T, typename... Args>
// struct func_traits<T, decltype(&T::template operator()<Args...>, void())>
//     : public func_traits<decltype(&T::template operator()<Args...>)> {};

namespace foo {
struct Bar {
  void go(int, float) {}
  struct run {
    void operator()(int, float, double){};
  };
};
} // namespace foo

template <typename T> void go_temp() {}

int main() {
  foo::Bar bar;
  func_traits<decltype(&foo::Bar::go)> a;
  printf("%lu\n", a.arg_count);
  func_traits<foo::Bar::run> b;
  std::function<void(int)> fa([](int) {});
  func_traits<decltype(fa)> c;
  auto fb = [](auto lhs, auto rhs) { return lhs < rhs; };
  func_traits<decltype(&go_temp<int>)> d;
  return 0;
}

