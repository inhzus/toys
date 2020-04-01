#ifndef TOYS_STREAM_SINK_H
#define TOYS_STREAM_SINK_H

#include <cassert>
#include <cstddef>
#include <functional>
#include <vector>

#include "traits.h"

template <typename T> class Sink {
public:
  Sink() : next_(nullptr) {}
  virtual ~Sink() = default;
  virtual void Pre(size_t len) = 0;
  virtual void Accept(const T &val) = 0;
  virtual void Post() = 0;
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

protected:
};

template <typename T> class HeadSink : public BasicSink<T> {
public:
  void Pre(size_t len) final { this->next_->Pre(len); }
  void Accept(const T &val) final { this->next_->Accept(val); }
  void Post() final { this->next_->Post(); }
};

template <typename T, typename Func> class MapSink : public BasicSink<T> {
public:
  MapSink(Func func) : BasicSink<T>(), func_(func) {
    static_assert(func_args_decay_to_v<Func, T>);
    static_assert(func_return_as_v<Func, T>);
  }
  void Pre(size_t len) final { this->next_->Pre(len); }
  void Accept(const T &val) final { this->next_->Accept(func_(val)); }
  void Post() final { this->next_->Post(); }

  Func func_;
};

template <typename T, typename Func> class FilterSink : public BasicSink<T> {
public:
  FilterSink(Func func) : BasicSink<T>(), func_(func) {
    static_assert(func_args_decay_to_v<Func, T>);
    static_assert(func_return_as_v<Func, bool>);
  }

  void Pre(size_t len) final { this->next_->Pre(len); }
  void Accept(const T &val) final {
    if (func_(val)) {
      this->next_->Accept(val);
    }
  }
  void Post() final { this->next_->Post(); }

  Func func_;
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

template <typename T, typename Func>
class FindFirstSink : public BreakableSink<T> {
public:
  FindFirstSink(Func func) : BreakableSink<T>(), func_(std::move(func)) {
    static_assert(func_args_decay_to_v<Func, T>);
    static_assert(func_return_as_v<Func, bool>);
  };
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
