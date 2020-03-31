#ifndef TOYS_STREAM_RANGE_H
#define TOYS_STREAM_RANGE_H

#include <cstddef>
#include <functional>
#include <type_traits>
#include <vector>

template <typename T> class Range {
public:
  virtual ~Range() = default;

  virtual const T &Next() = 0;
  [[nodiscard]] virtual bool Valid() const = 0;
  [[nodiscard]] virtual size_t Size() const = 0;
};

template <typename It,
          typename T = std::remove_reference_t<decltype(*std::declval<It>())>>
class BasicRange : public Range<T> {
public:
  BasicRange(It first, It last) : first_(first), last_(last), cur_(first_) {}
  const T &Next() override {
    const T &val = *cur_;
    ++cur_;
    return val;
  }
  [[nodiscard]] bool Valid() const final { return cur_ != last_; }
  [[nodiscard]] size_t Size() const final { return last_ - first_; }

private:
  It first_, last_, cur_;
};

template <typename U, typename T = std::remove_cv_t<std::remove_reference_t<U>>>
class StepRange : public Range<T> {
public:
  StepRange(T start, T stop, std::function<T(U)> func)
      : start_(std::move(start)), stop_(std::move(stop)), val_(start),
        next_(start), func_(std::move(func)) {}
  const T &Next() final {
    auto tmp = func_(next_);
    std::swap(val_, next_);
    std::swap(next_, tmp);
    return val_;
  }
  [[nodiscard]] bool Valid() const final { return val_ < stop_; }
  [[nodiscard]] size_t Size() const final { return 0; }

private:
  T start_, stop_, val_, next_;
  std::function<T(U)> func_;
};

#endif
