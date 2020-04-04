//
// Copyright [2020] <inhzus>
//
#ifndef TOYS_STREAM_STEP_RANGE_H_
#define TOYS_STREAM_STEP_RANGE_H_

#include <cstddef>
#include <functional>
#include <type_traits>
#include <vector>

template <typename T, typename ValidFunc, typename StepFunc>
class StepRange {
 public:
  class Iterator {
   public:
    explicit Iterator(const StepRange *range) : range_(range) {
      cur_ = range_ == nullptr ? nullptr : new T(range_->start_);
    }
    Iterator(const Iterator &it) : range_(it.range_) {
      cur_ = it.cur_ == nullptr ? nullptr : new T(it.cur_);
    }
    Iterator &operator=(const Iterator &it) {
      range_ = it.range_;
      if (cur_ != nullptr) delete cur_;
      cur_ = it.cur_ == nullptr ? nullptr : new T(it.cur_);
      return *this;
    }
    ~Iterator() {
      if (cur_ != nullptr) delete cur_;
    }

    bool operator==(const Iterator &it) const {
      if (it.range_ == nullptr) {
        if (range_ == nullptr) return true;
        return !range_->valid_func_(*cur_);
      }
      return range_ == it.range_ && cur_ == it.cur_;
    }
    bool operator!=(const Iterator &it) const { return !operator==(it); }
    T &operator*() { return *cur_; }
    T *operator->() { return cur_; }
    T &operator++() {
      range_->step_func_(cur_);
      return *cur_;
    }
    T operator++(int) {
      T tmp(*cur_);
      range_->step_func_(cur_);
      return tmp;
    }

   private:
    const StepRange *range_;
    T *cur_;
  };

  template <typename Step>
  StepRange(T start, T stop, Step step)
      : start_(std::move(start)),
        valid_func_(
            [stop = std::move(stop)](const T &val) { return val != stop; }),
        step_func_([step = std::move(step)](T *val) { *val += step; }) {}
  StepRange(T start, T stop, StepFunc step_func)
      : start_(std::move(start)),
        valid_func_(
            [stop = std::move(stop)](const T &val) { return val != stop; }),
        step_func_(std::move(step_func)) {}
  template <typename Step>
  StepRange(T start, ValidFunc valid_func, Step step)
      : start_(std::move(start)),
        valid_func_(std::move(valid_func)),
        step_func_([step = std::move(step)](T *val) { *val += step; }) {}
  StepRange(T start, ValidFunc valid_func, StepFunc step_func)
      : start_(std::move(start)),
        valid_func_(std::move(valid_func)),
        step_func_(std::move(step_func)) {}
  StepRange(StepRange &&) = default;
  StepRange &operator=(StepRange &&) = default;

  Iterator begin() const { return Iterator(this); }
  Iterator end() const { return Iterator(nullptr); }
  [[nodiscard]] size_t size() const { return 0; }

 private:
  T start_;
  ValidFunc valid_func_;
  StepFunc step_func_;
};

template <typename T, typename Step,
          std::enable_if_t<std::is_same_v<T, decltype(std::declval<T>() +
                                                      std::declval<Step>())>,
                           int> = 0>
StepRange(T, T, Step)
    ->StepRange<T, std::function<bool(const T &)>, std::function<void(T *)>>;

template <typename T, typename StepFunc,
          std::enable_if_t<std::is_invocable_r_v<void, StepFunc, T *>, int> = 0>
StepRange(T, T, StepFunc)
    ->StepRange<T, std::function<bool(const T &)>, StepFunc>;

template <
    typename T, typename ValidFunc, typename Step,
    std::enable_if_t<std::is_invocable_r_v<bool, ValidFunc, const T &> &&
                         std::is_same_v<T, decltype(std::declval<T>() +
                                                    std::declval<Step>())>,
                     int> = 0>
StepRange(T, ValidFunc, Step)
    ->StepRange<T, ValidFunc, std::function<void(T *)>>;

#endif  // TOYS_STREAM_STEP_RANGE_H_
