#ifndef TOYS_STREAM_SINK_H
#define TOYS_STREAM_SINK_H

#include <cassert>
#include <cstddef>
#include <functional>
#include <vector>

#include "traits.h"

template <typename, typename> class Stream;

template <typename T> class Sink {
public:
  Sink() : next_(nullptr) {}
  virtual ~Sink() = default;
  virtual void Pre(size_t len) = 0;
  virtual void Accept(const T &val) = 0;
  virtual void Post() = 0;

  template <typename R> void Evaluate(const R &range) {
    auto *recv = static_cast<Sink<value_type_of<R>> *>(Reciever());
    recv->Pre(range.size());
    for (const auto &val : range) {
      if (recv->Cancelled())
        break;
      recv->Accept(val);
    }
    recv->Post();
  }

  [[nodiscard]] virtual bool Cancelled() const = 0;
  void set_next(Sink *next) { next_ = next; }

protected:
  virtual void *Reciever() { return this; };
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

template <typename R, typename T, typename U>
class CastSink : public BasicSink<U> {
public:
  CastSink(Stream<R, T> &&stream) : stream_(std::move(stream)) {}
  void Pre(size_t len) final { this->next_->Pre(len); };
  void Accept(const U &val) final { this->next_->Accept(val); };
  void Post() final{this->next_->Post();};
  Stream<R, T> &stream() { return stream_; }

private:
  void *Reciever() final;
  Stream<R, T> stream_;
};

template <typename T, typename Func> class MapSink : public BasicSink<T> {
public:
  MapSink(Func func) : BasicSink<T>(), func_(std::move(func)) {}
  void Pre(size_t len) final { this->next_->Pre(len); }
  void Accept(const T &val) final { this->next_->Accept(func_(val)); }
  void Post() final { this->next_->Post(); }

private:
  Func func_;
};

template <typename T, typename Func> class FlatMapSink : public BasicSink<T> {
public:
  FlatMapSink(Func func) : BasicSink<T>(), func_(std::move(func)) {}
  void Pre(size_t len) final { this->next_->Pre(0); }
  void Accept(const T &val) final {
    auto container = func_(val);
    for (const auto &item : container) {
      this->next_->Accept(item);
    }
  }
  void Post() final { this->next_->Post(); }

private:
  Func func_;
};

template <typename T, typename Func> class FilterSink : public BasicSink<T> {
public:
  FilterSink(Func func) : BasicSink<T>(), func_(std::move(func)) {}

  void Pre(size_t len) final { this->next_->Pre(len); }
  void Accept(const T &val) final {
    if (func_(val)) {
      this->next_->Accept(val);
    }
  }
  void Post() final { this->next_->Post(); }

private:
  Func func_;
};

template <typename T, typename Less> class SortSink : public BasicSink<T> {
public:
  SortSink(Less less) : less_(std::move(less)), vals_() {}

  void Pre(size_t len) { vals_.reserve(len); }
  void Accept(const T &val) { vals_.push_back(val); }
  void Post() {
    std::sort(vals_.begin(), vals_.end(), less_);
    this->next_->Evaluate(vals_);
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

template <typename T, typename Func> class ForEachSink : public FinalSink<T> {
public:
  ForEachSink(Func func) : FinalSink<T>(), func_(std::move(func)) {}
  void Pre(size_t len) final {}
  void Accept(const T &val) final { func_(val); }
  void Post() final {}

private:
  Func func_;
};

template <typename T, typename Select> class MostSink : public FinalSink<T> {
public:
  MostSink(Select most)
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
  Select select_;
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

private:
  CastSink<R, T, U> *cast_;
  Func func_;
};

template <typename T, typename Func>
class FindFirstSink : public BreakableSink<T> {
public:
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
