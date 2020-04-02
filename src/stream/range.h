//
// Copyright [2020] <inhzus>
//
#ifndef TOYS_STREAM_RANGE_H
#define TOYS_STREAM_RANGE_H

#include <cstddef>
#include <functional>
#include <type_traits>
#include <vector>

#include "traits.h"

template <typename T> class Stream;

template <typename T> class Range {
public:
  virtual ~Range() = default;

  virtual const T &Next() = 0;
  [[nodiscard]] virtual bool Valid() const = 0;
  [[nodiscard]] virtual size_t Size() const = 0;

  Stream<T> Stream();
};

template <typename It, typename T = std::decay_t<decltype(*std::declval<It>())>>
class ItRange : public Range<T> {
public:
  ItRange(It first, It last) : first_(first), last_(last), cur_(first_) {}
  const T &Next() final {
    T &val = *cur_;
    ++cur_;
    return val;
  }
  [[nodiscard]] bool Valid() const final { return cur_ != last_; }
  [[nodiscard]] size_t Size() const final { return last_ - first_; }

private:
  It first_, last_, cur_;
};

template <typename T, typename StopFunc, typename StepFunc>
class StepRange : public Range<T> {
public:
  template <
      std::enable_if_t<std::is_invocable_r_v<T, StepFunc, const T &>, int> = 0>
  StepRange(T start, T stop, StepFunc func)
      : start_(std::move(start)),
        stop_([stop](const T &val) { return val == stop; }), val_(start),
        next_(start), step_(std::move(func)) {}
  template <typename Step>
  StepRange(T start, T stop, Step step)
      : start_(std::move(start)),
        stop_([stop](const T &val) { return val == stop; }), val_(start),
        next_(start), step_([step](const T &val) { return val + step; }) {}

  const T &Next() final {
    auto tmp = step_(next_);
    std::swap(val_, next_);
    std::swap(next_, tmp);
    return val_;
  }
  [[nodiscard]] bool Valid() const final { return !stop_(val_); }
  [[nodiscard]] size_t Size() const final { return 0; }

private:
  T start_, val_, next_;
  StopFunc stop_;
  StepFunc step_;
};

template <
    typename T, typename Step,
    typename std::enable_if_t<
        std::is_same_v<T, decltype(std::declval<T>() + std::declval<Step>())>,
        int> = 0>
StepRange(T, T, Step)
    ->StepRange<T, std::function<bool(const T &)>, std::function<T(const T &)>>;

#endif
