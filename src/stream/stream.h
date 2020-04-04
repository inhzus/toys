#ifndef TOYS_STREAM_STREAM_H_
#define TOYS_STREAM_STREAM_H_

#include <functional>
#include <vector>

#include "sink.h"
#include "step_range.h"

template <typename R, typename T = std::decay_t<decltype(
                          *std::declval<std::remove_pointer_t<R>>().begin())>>
class Stream {
 public:
  template <typename, typename, typename>
  friend class CastSink;
  template <typename, typename>
  friend class Stream;
  Stream(R &&range) : range_(std::move(range)) {
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
    sinks_.push_back(new HeadSink<T>());
  }
  Stream(Stream &&) = default;
  virtual ~Stream() {
    for (auto p : sinks_) {
      delete p;
    }
  };
  template <typename Func,
            typename U = std::decay_t<std::invoke_result_t<Func, const T &>>,
            std::enable_if_t<std::is_same_v<T, U>, int> = 0>
  Stream &Map(Func func) {
    static_assert(std::is_invocable_r_v<T, Func, const T &>);
    sinks_.push_back(new MapSink<T, Func>(std::move(func)));
    return *this;
  }
  template <typename Func,
            typename U = std::decay_t<std::invoke_result_t<Func, const T &>>,
            std::enable_if_t<!std::is_same_v<T, U>, int> = 0>
  Stream<R, U> Map(Func func) {
    static_assert(std::is_invocable_r_v<U, Func, const T &>);
    auto *cast = new CastSink<R, T, U>(std::move(*this));
    auto &elder = cast->stream();
    elder.sinks_.push_back(
        new MapObjSink<R, T, U, Func>(cast, std::move(func)));
    elder.MakeChain();
    Stream<R, U> stream(std::move(elder.range_), cast);
    return stream;
  }
  template <typename Func>
  Stream &FlatMap(Func func) {
    sinks_.push_back(new FlatMapSink<T, Func>(std::move(func)));
    return *this;
  }
  template <typename Func>
  Stream &Filter(Func func) {
    static_assert(std::is_invocable_r_v<bool, Func, const T &>);
    sinks_.push_back(new FilterSink<T, Func>(std::move(func)));
    return *this;
  }
  template <typename Less = std::less<T>>
  Stream &Sort(Less less = Less()) {
    static_assert(std::is_invocable_r_v<bool, Less, const T &, const T &>);
    sinks_.push_back(new SortSink<T, Less>(std::move(less)));
    return *this;
  }
  std::vector<T> Collect() {
    auto sink = new CollectSink<T>();
    sinks_.push_back(sink);
    Evaluate();
    std::vector<T> vals(std::move(sink->vals()));
    return vals;
  }
  template <typename Func>
  void ForEach(Func func) {
    static_assert(std::is_invocable_r_v<void, Func, const T &>);
    auto sink = new ForEachSink<T, Func>(std::move(func));
    sinks_.push_back(sink);
    Evaluate();
  }
  template <typename Select>
  T Most(Select most) {
    static_assert(std::is_invocable_r_v<T, Select, const T &, const T &>);
    auto sink = new MostSink<T, Select>(std::move(most));
    sinks_.push_back(sink);
    Evaluate();
    return std::move(sink->val());
  }
  template <typename Func>
  std::pair<bool, T> FindFirst(Func func) {
    static_assert(std::is_invocable_r_v<bool, Func, const T &>);
    auto sink = new FindFirstSink<T, Func>(std::move(func));
    sinks_.push_back(sink);
    Evaluate();
    return {sink->Cancelled(), std::move(sink->val())};
  }

 private:
  Stream(R &&range, Sink<T> *sink) : range_(std::move(range)), sinks_{sink} {}
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
      sinks_[i]->set_next(sinks_[i + 1]);
    }
  }
  R range_;
  std::vector<Sink<T> *> sinks_;
};

template <typename R, typename T, typename U>
void *CastSink<R, T, U>::Reciever() {
  return stream_.sinks_[0]->Reciever();
}

#endif  // TOYS_STREAM_STREAM_H_
