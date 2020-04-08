//
// Copyright [2020] <inhzus>
//
#ifndef TOYS_STREAM_SINK_H_
#define TOYS_STREAM_SINK_H_

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "./traits.h"

template <typename, typename>
class Stream;

template <typename T>
class Sink {
 public:
  Sink() : next_(nullptr) {}
  virtual ~Sink() = default;
  virtual void Pre(size_t len) = 0;
  virtual void Accept(const T &val) = 0;
  virtual void Post() = 0;
  virtual void *Reciever() { return this; }

  template <typename R>
  void Evaluate(const R &range) {
    auto *recv = static_cast<Sink<value_type_of<R>> *>(Reciever());
    recv->Pre(range.size());
    for (const auto &val : range) {
      if (recv->Cancelled()) break;
      recv->Accept(val);
    }
    recv->Post();
  }

  [[nodiscard]] virtual bool Cancelled() const = 0;
  void set_next(Sink *next) { next_ = next; }

 protected:
  Sink<T> *next_;
};

template <typename T>
class BasicSink : public Sink<T> {
 public:
  [[nodiscard]] bool Cancelled() const override {
    return this->next_->Cancelled();
  }
};

template <typename T>
class HeadSink : public BasicSink<T> {
 public:
  void Pre(size_t len) final { this->next_->Pre(len); }
  void Accept(const T &val) final { this->next_->Accept(val); }
  void Post() final { this->next_->Post(); }
};

template <typename R, typename T, typename U>
class CastSink : public BasicSink<U> {
 public:
  explicit CastSink(Stream<R, T> &&stream) : stream_(std::move(stream)) {}
  void Pre(size_t len) final { this->next_->Pre(len); };
  void Accept(const U &val) final { this->next_->Accept(val); };
  void Post() final { this->next_->Post(); };
  void *Reciever() final;
  Stream<R, T> &stream() { return stream_; }

 private:
  Stream<R, T> stream_;
};

template <typename T, typename Func>
class MapSink : public BasicSink<T> {
 public:
  explicit MapSink(Func func) : BasicSink<T>(), func_(std::move(func)) {}
  void Pre(size_t len) final { this->next_->Pre(len); }
  void Accept(const T &val) final { this->next_->Accept(func_(val)); }
  void Post() final { this->next_->Post(); }

 private:
  Func func_;
};

template <typename T, typename Func>
class FlatMapSink : public BasicSink<T> {
 public:
  explicit FlatMapSink(Func func) : BasicSink<T>(), func_(std::move(func)) {}
  void Pre(size_t len) final { this->next_->Pre(0); }
  void Accept(const T &val) final {
    auto container = func_(val);
    for (const auto &item : container) {
      if (this->next_->Cancelled()) return;
      this->next_->Accept(item);
    }
  }
  void Post() final { this->next_->Post(); }

 private:
  Func func_;
};

template <typename T, typename Func>
class FilterSink : public BasicSink<T> {
 public:
  explicit FilterSink(Func func) : BasicSink<T>(), func_(std::move(func)) {}

  void Pre(size_t len) final { this->next_->Pre(0); }
  void Accept(const T &val) final {
    if (func_(val)) {
      this->next_->Accept(val);
    }
  }
  void Post() final { this->next_->Post(); }

 private:
  Func func_;
};

template <typename T, typename Func>
class PeekSink : public BasicSink<T> {
 public:
  explicit PeekSink(Func func) : BasicSink<T>(), func_(std::move(func)) {}

  void Pre(size_t len) final { this->next_->Pre(len); }
  void Accept(const T &val) final {
    func_(val);
    this->next_->Accept(val);
  }
  void Post() final { this->next_->Post(); }

 private:
  Func func_;
};

// stateful sinks

template <typename T, typename Less>
class SortSink : public BasicSink<T> {
 public:
  explicit SortSink(Less less) : less_(std::move(less)), vals_() {}

  void Pre(size_t len) final { vals_.reserve(len); }
  void Accept(const T &val) final { vals_.emplace_back(val); }
  void Post() final {
    std::sort(vals_.begin(), vals_.end(), less_);
    this->next_->Evaluate(vals_);
  }

 private:
  Less less_;
  std::vector<T> vals_;
};

template <typename T>
class LimitSink : public BasicSink<T> {
 public:
  explicit LimitSink(size_t max) : BasicSink<T>(), cnt_(0), max_(max) {}
  void Pre(size_t len) final { this->next_->Pre(std::min(len, max_)); }
  void Accept(const T &val) final {
    if (cnt_ < max_) {
      ++cnt_;
      this->next_->Accept(val);
    }
  }
  void Post() final { this->next_->Post(); }
  [[nodiscard]] bool Cancelled() const final {
    return cnt_ >= max_ || this->next_->Cancelled();
  }

 private:
  size_t cnt_;
  size_t max_;
};

template <typename T>
class SkipSink : public BasicSink<T> {
 public:
  explicit SkipSink(size_t skip) : BasicSink<T>(), cnt_(0), skip_(skip) {}
  void Pre(size_t len) final {
    len = len > skip_ ? len - skip_ : 0;
    this->next_->Pre(len);
  }
  void Accept(const T &val) final {
    if (cnt_ < skip_) {
      ++cnt_;
    } else {
      this->next_->Accept(val);
    }
  }
  void Post() final { this->next_->Post(); }

 private:
  size_t cnt_;
  size_t skip_;
};

template <typename T, typename Hash>
class DistinctSink : public BasicSink<T> {
 public:
  explicit DistinctSink(Hash hash) : set_(0, std::move(hash)) {}
  void Pre(size_t) final { this->next_->Pre(0); }
  void Accept(const T &val) final {
    if (set_.insert(val).second) {
      this->next_->Accept(val);
    }
  }
  void Post() final { this->next_->Post(); }

 private:
  std::unordered_set<T, Hash> set_;
};

template <typename T>
class FinalSink : public Sink<T> {
 public:
  [[nodiscard]] bool Cancelled() const override { return false; }
};

template <typename T>
class BreakableSink : public FinalSink<T> {
 public:
  BreakableSink() : FinalSink<T>(), cancelled_(false) {}
  [[nodiscard]] bool Cancelled() const final { return cancelled_; }

 protected:
  bool cancelled_;
};

template <typename T>
class CollectSink : public FinalSink<T> {
 public:
  void Pre(size_t len) final { vals_.reserve(len); }
  void Accept(const T &val) final { vals_.emplace_back(val); }
  void Post() final {}
  std::vector<T> &vals() { return vals_; }

 private:
  std::vector<T> vals_;
};

template <typename T, typename Func>
class ForEachSink : public FinalSink<T> {
 public:
  explicit ForEachSink(Func func) : FinalSink<T>(), func_(std::move(func)) {}
  void Pre(size_t len) final {}
  void Accept(const T &val) final { func_(val); }
  void Post() final {}

 private:
  Func func_;
};

template <typename T, typename Func>
class ReduceSink : public FinalSink<T> {
 public:
  explicit ReduceSink(Func most)
      : FinalSink<T>(), is_first_(true), select_(std::move(most)) {}

  void Pre(size_t len) final {}
  void Accept(const T &val) final {
    if (is_first_) {
      val_ = val;
      is_first_ = false;
    } else {
      decltype(auto) tmp = select_(val_, val);
      std::swap(val_, tmp);
    }
  }
  void Post() final {}

  T &val() { return val_; }

 private:
  bool is_first_;
  Func select_;
  T val_;
};

template <typename R, typename T, typename U, typename Func>
class MapObjSink : public FinalSink<T> {
 public:
  MapObjSink(CastSink<R, T, U> *cast, Func func)
      : FinalSink<T>(), cast_(cast), func_(std::move(func)) {}
  void Pre(size_t len) final { cast_->Pre(len); }
  void Accept(const T &val) final { cast_->Accept(func_(val)); }
  void Post() final { cast_->Post(); }
  [[nodiscard]] bool Cancelled() const final { return cast_->Cancelled(); }

 private:
  CastSink<R, T, U> *cast_;
  Func func_;
};

template <typename R, typename T, typename U, typename Func>
class FlatMapObjSink : public FinalSink<T> {
 public:
  FlatMapObjSink(CastSink<R, T, U> *cast, Func func)
      : FinalSink<T>(), cast_(cast), func_(std::move(func)) {}
  void Pre(size_t len) final { cast_->Pre(0); }
  void Accept(const T &val) final {
    decltype(auto) vals = func_(val);
    for (const auto &val : vals) {
      if (cast_->Cancelled()) return;
      cast_->Accept(val);
    }
  }
  void Post() final { cast_->Post(); }
  [[nodiscard]] bool Cancelled() const final { return cast_->Cancelled(); }

 private:
  CastSink<R, T, U> *cast_;
  Func func_;
};

template <typename T, typename Func>
class FindFirstSink : public BreakableSink<T> {
 public:
  explicit FindFirstSink(Func func)
      : BreakableSink<T>(), func_(std::move(func)) {}
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

template <typename T>
class CountSink : public BreakableSink<T> {
 public:
  CountSink() : BreakableSink<T>(), cnt_(0) {}
  void Pre(size_t len) final {
    if (len != 0) {
      cnt_ = len;
      this->cancelled_ = true;
    }
  }
  void Accept(const T &) final { ++cnt_; }
  void Post() final {}
  size_t cnt() { return cnt_; }

 private:
  size_t cnt_;
};

#endif  // TOYS_STREAM_SINK_H_
