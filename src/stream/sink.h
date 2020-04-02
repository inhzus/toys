#ifndef TOYS_STREAM_SINK_H
#define TOYS_STREAM_SINK_H

#include <cassert>
#include <cstddef>
#include <functional>
#include <vector>

#include "range.h"
#include "traits.h"

template <typename T> class Sink {
public:
  Sink() : next_(nullptr) {}
  virtual ~Sink() = default;
  virtual void Pre(size_t len) = 0;
  virtual void Accept(const T &val) = 0;
  virtual void Post() = 0;

  void Evaluate(Range<T> *range) {
    this->Pre(range->Size());
    while (range->Valid()) {
      if (this->Cancelled())
        break;
      this->Accept(range->Next());
    }
    this->Post();
  }

  [[nodiscard]] virtual bool Cancelled() const = 0;
  void set_next(Sink *next) { next_ = next; }

protected:
  Sink<T> *next_;
};

template <typename T> class BasicSink : public Sink<T> {
public:
  [[nodiscard]] bool Cancelled() const override {
    return this->next_->Cancelled();
  }
};

template <typename T> class HeadSink : public BasicSink<T> {
public:
  void Pre(size_t len) final { this->next_->Pre(len); }
  void Accept(const T &val) final { this->next_->Accept(val); }
  void Post() final { this->next_->Post(); }
};

template <typename T, typename Func> class MapSink : public BasicSink<T> {
public:
  template <typename std::enable_if_t<std::is_invocable_r_v<T, Func, const T &>,
                                      int> = 0>
  MapSink(Func func) : BasicSink<T>(), func_(func) {}
  void Pre(size_t len) final { this->next_->Pre(len); }
  void Accept(const T &val) final { this->next_->Accept(func_(val)); }
  void Post() final { this->next_->Post(); }

  Func func_;
};

template <typename T, typename Func> class FilterSink : public BasicSink<T> {
public:
  template <typename std::enable_if_t<
                std::is_invocable_r_v<bool, Func, const T &>, int> = 0>
  FilterSink(Func func) : BasicSink<T>(), func_(func) {}

  void Pre(size_t len) final { this->next_->Pre(len); }
  void Accept(const T &val) final {
    if (func_(val)) {
      this->next_->Accept(val);
    }
  }
  void Post() final { this->next_->Post(); }

  Func func_;
};

template <typename T, typename Less> class SortSink : public BasicSink<T> {
public:
  template <
      typename std::enable_if_t<
          std::is_invocable_r_v<bool, Less, const T &, const T &>, int> = 0>
  SortSink(Less less) : less_(std::move(less)), vals_() {}

  void Pre(size_t len) { vals_.reserve(len); }
  void Accept(const T &val) { vals_.push_back(val); }
  void Post() {
    std::sort(vals_.begin(), vals_.end(), less_);
    auto range = BasicRange(vals_.begin(), vals_.end());
    this->next_->Evaluate(&range);
  }

private:
  Less less_;
  std::vector<T> vals_;
};

template <typename T> class FinalSink : public Sink<T> {
public:
  [[nodiscard]] bool Cancelled() const override { return false; }
};

template <typename T> class BreakableSink : public FinalSink<T> {
public:
  BreakableSink() : FinalSink<T>(), cancelled_(false) {}
  [[nodiscard]] bool Cancelled() const final { return cancelled_; }

protected:
  bool cancelled_;
};

template <typename T> class CollectSink : public FinalSink<T> {
public:
  void Pre(size_t len) final { vals_.reserve(len); }
  void Accept(const T &val) final { vals_.emplace_back(val); }
  void Post() final {}
  std::vector<T> &vals() { return vals_; }

private:
  std::vector<T> vals_;
};

template <typename T, typename Most> class MostSink : public FinalSink<T> {
public:
  template <typename std::enable_if_t<
                std::is_invocable_r_v<T, Most, const T &, const T &>, int> = 0>
  MostSink(Most most)
      : FinalSink<T>(), is_first_(true), most_(std::move(most)) {}

  void Pre(size_t len) final {}
  void Accept(const T &val) final {
    if (is_first_) {
      val_ = val;
      is_first_ = false;
    } else {
      decltype(auto) tmp = most_(val_, val);
      std::swap(val_, tmp);
    }
  }
  void Post() final {}

  T &val() { return val_; }

private:
  bool is_first_;
  Most most_;
  T val_;
};

template <typename T, typename Func>
class FindFirstSink : public BreakableSink<T> {
public:
  template <typename std::enable_if_t<
                std::is_invocable_r_v<bool, Func, const T &>, int> = 0>
  FindFirstSink(Func func) : BreakableSink<T>(), func_(std::move(func)){};
  void Pre(size_t len) final {}
  void Accept(const T &val) final {
    if (func_(val)) {
      val_ = val;
      this->cancelled_ = true;
    }
  }
  void Post() {}
  T &val() { return val_; }

private:
  Func func_;
  T val_;
};

#endif // TOYS_STREAM_SINK_H
