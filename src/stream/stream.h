//
// Copyright [2020] <inhzus>
//
#ifndef TOYS_STREAM_STREAM_H_
#define TOYS_STREAM_STREAM_H_

#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <utility>
#include <vector>

#include "./sink.h"
#include "./step_range.h"

template <typename R, typename T = std::decay_t<decltype(
                          *std::declval<std::remove_pointer_t<R>>().begin())>>
class Stream {
 public:
  template <typename, typename, typename>
  friend class CastSink;
  template <typename, typename>
  friend class Stream;

  class Iterator {
   public:
    explicit Iterator(Stream<R, T> *stream)
        : stop_(false),
          stream_(stream),
          head_(static_cast<Sink<value_type_of<R>> *>(
              stream->sinks_[0]->Reciever())),
          it_(stream_->range_.begin()) {
      stream_->sinks_.emplace_back(
          std::make_unique<ForEachSink<T, std::function<void(const T &)>>>(
              [&buf = buf_](const T &val) { buf.emplace(val); }));
      stream_->MakeChain();
      head_->Pre(stream_->range_.size());
      LoadNext();
    }
    explicit Iterator(std::nullptr_t)
        : stop_(false), stream_(nullptr), head_(nullptr), it_() {}
    bool operator==(const Iterator &it) const {
      if (it.stream_ == nullptr) {
        if (stream_ == nullptr) return true;
        return buf_.empty() && stop_;
      }
      // not expected
      return true;
    }
    bool operator!=(const Iterator &it) const { return !operator==(it); }
    T &operator*() { return buf_.front(); }
    T *operator->() { return &buf_.front(); }
    T &operator++() {
      buf_.pop();
      LoadNext();
      return buf_.front();
    }
    T operator++(int) {
      T tmp = buf_.front();
      ++*this;
      return tmp;
    }

   private:
    void LoadNext() {
      while (buf_.empty()) {
        if (stop_) return;
        if (it_ == stream_->range_.end()) {
          head_->Post();
          stop_ = true;
          return;
        }
        head_->Accept(*it_);
        ++it_;
      }
    }

    bool stop_;
    Stream<R, T> *stream_;
    Sink<value_type_of<R>> *head_;
    std::decay_t<decltype(std::declval<std::remove_pointer_t<R>>().begin())>
        it_;
    std::queue<T> buf_;
  };

  explicit Stream(R &&range) : range_(std::move(range)) {
    using Container = std::remove_pointer_t<R>;
    using U = value_type_of<Container>;
    static_assert(
        std::is_same_v<
            U, std::decay_t<decltype(*std::declval<Container>().begin())>>);
    static_assert(std::is_same_v<
                  U, std::decay_t<decltype(*std::declval<Container>().end())>>);
    static_assert(
        std::is_convertible_v<
            std::decay_t<decltype(std::declval<Container>().size())>, size_t>);
    sinks_.emplace_back(std::make_unique<HeadSink<T>>());
  }
  Stream(const Stream &) = delete;
  Stream(Stream &&) = default;
  Stream &operator=(const Stream &) = delete;
  Stream &operator=(Stream &&) = default;
  template <typename Func,
            typename U = std::decay_t<std::invoke_result_t<Func, const T &>>,
            std::enable_if_t<std::is_same_v<T, U>, int> = 0>
  Stream Map(Func func) {
    static_assert(std::is_invocable_r_v<T, Func, const T &>);
    sinks_.emplace_back(std::make_unique<MapSink<T, Func>>(std::move(func)));
    return std::move(*this);
  }
  template <typename Func,
            typename U = std::decay_t<std::invoke_result_t<Func, const T &>>,
            std::enable_if_t<!std::is_same_v<T, U>, int> = 0>
  Stream<R, U> Map(Func func) {
    auto cast = std::make_unique<CastSink<R, T, U>>(std::move(*this));
    auto &elder = cast->stream();
    elder.sinks_.emplace_back(std::make_unique<MapObjSink<R, T, U, Func>>(
        cast.get(), std::move(func)));
    elder.MakeChain();
    Stream<R, U> stream(std::move(elder.range_), std::move(cast));
    return stream;
  }
  template <typename Func,
            typename U = value_type_of<std::invoke_result_t<Func, const T &>>,
            std::enable_if_t<std::is_same_v<T, U>, int> = 0>
  Stream FlatMap(Func func) {
    sinks_.emplace_back(
        std::make_unique<FlatMapSink<T, Func>>(std::move(func)));
    return std::move(*this);
  }
  template <typename Func,
            typename U = value_type_of<std::invoke_result_t<Func, const T &>>,
            std::enable_if_t<!std::is_same_v<T, U>, int> = 0>
  Stream<R, U> FlatMap(Func func) {
    auto cast = std::make_unique<CastSink<R, T, U>>(std::move(*this));
    auto &elder = cast->stream();
    elder.sinks_.emplace_back(std::make_unique<FlatMapObjSink<R, T, U, Func>>(
        cast.get(), std::move(func)));
    elder.MakeChain();
    Stream<R, U> stream(std::move(elder.range_), std::move(cast));
    return stream;
  }
  template <typename Func>
  Stream Filter(Func func) {
    static_assert(std::is_invocable_r_v<bool, Func, const T &>);
    sinks_.emplace_back(std::make_unique<FilterSink<T, Func>>(std::move(func)));
    return std::move(*this);
  }
  template <typename Func>
  Stream Peek(Func func) {
    static_assert(std::is_invocable_v<Func, const T &>);
    sinks_.emplace_back(std::make_unique<PeekSink<T, Func>>(std::move(func)));
    return std::move(*this);
  }
  template <typename Less = std::less<T>>
  Stream Sort(Less less = Less()) {
    static_assert(std::is_invocable_r_v<bool, Less, const T &, const T &>);
    sinks_.emplace_back(std::make_unique<SortSink<T, Less>>(std::move(less)));
    return std::move(*this);
  }
  Stream Limit(size_t max) {
    sinks_.emplace_back(std::make_unique<LimitSink<T>>(max));
    return std::move(*this);
  }
  Stream Skip(size_t skip) {
    sinks_.emplace_back(std::make_unique<SkipSink<T>>(skip));
    return std::move(*this);
  }
  template <typename Hash = std::hash<T>>
  Stream Distinct(Hash hash = Hash()) {
    static_assert(std::is_invocable_r_v<size_t, Hash, const T &>);
    sinks_.emplace_back(
        std::make_unique<DistinctSink<T, Hash>>(std::move(hash)));
    return std::move(*this);
  }
  std::vector<T> Collect() {
    auto *sink = new CollectSink<T>();
    sinks_.emplace_back(std::unique_ptr<CollectSink<T>>(sink));
    Evaluate();
    std::vector<T> vals(std::move(sink->vals()));
    return vals;
  }
  template <typename Func>
  void ForEach(Func func) {
    static_assert(std::is_invocable_r_v<void, Func, const T &>);
    sinks_.emplace_back(
        std::make_unique<ForEachSink<T, Func>>(std::move(func)));
    Evaluate();
  }
  template <typename Func>
  T Reduce(Func most) {
    static_assert(std::is_invocable_r_v<T, Func, const T &, const T &>);
    auto *sink = new ReduceSink<T, Func>(std::move(most));
    sinks_.emplace_back(std::unique_ptr<ReduceSink<T, Func>>(sink));
    Evaluate();
    return std::move(sink->val());
  }
  template <typename Func>
  std::optional<T> FindFirst(Func func) {
    static_assert(std::is_invocable_r_v<bool, Func, const T &>);
    auto *sink = new FindFirstSink<T, Func>(std::move(func));
    sinks_.emplace_back(std::unique_ptr<FindFirstSink<T, Func>>(sink));
    Evaluate();
    return sink->Cancelled() ? std::optional<T>() : std::move(sink->val());
  }
  size_t Count() {
    auto *sink = new CountSink<T>();
    sinks_.emplace_back(std::unique_ptr<CountSink<T>>(sink));
    Evaluate();
    return sink->cnt();
  }

  Iterator begin() { return Iterator(this); }
  Iterator end() const { return Iterator(nullptr); }
  [[nodiscard]] size_t size() const { return 0; }

 private:
  Stream(R &&range, std::unique_ptr<Sink<T>> &&sink)
      : range_(std::move(range)) {
    sinks_.emplace_back(std::move(sink));
  }
  void Evaluate() {
    MakeChain();
    if constexpr (std::is_pointer_v<R>) {
      sinks_[0]->Evaluate(*range_);
    } else {
      sinks_[0]->Evaluate(range_);
    }
  }
  void MakeChain() {
    for (size_t i = 0; i + 1 < sinks_.size(); ++i) {
      sinks_[i]->set_next(sinks_[i + 1].get());
    }
  }
  R range_;
  std::vector<std::unique_ptr<Sink<T>>> sinks_;
};

template <typename R, typename T, typename U>
void *CastSink<R, T, U>::Reciever() {
  return stream_.sinks_[0]->Reciever();
}

#endif  // TOYS_STREAM_STREAM_H_
