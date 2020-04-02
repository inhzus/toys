#ifndef TOYS_STREAM_STREAM_H
#define TOYS_STREAM_STREAM_H

#include <functional>
#include <vector>

#include "range.h"
#include "sink.h"

template <typename T> class Stream {
public:
  Stream(Range<T> *range) : range_(range) {
    sinks_.push_back(new HeadSink<T>());
  }
  ~Stream() {
    for (auto p : sinks_) {
      delete p;
    }
  };
  template <typename Func> Stream &Map(Func func) {
    sinks_.push_back(new MapSink<T, Func>(std::move(func)));
    return *this;
  }
  template <typename Func> Stream &Filter(Func func) {
    sinks_.push_back(new FilterSink<T, Func>(std::move(func)));
    return *this;
  }
  template <typename Less = std::less<T>> Stream &Sort(Less less = Less()) {
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
  template <typename Func> void ForEach(Func func) {
    auto sink = new ForEachSink<T, Func>(std::move(func));
    sinks_.push_back(sink);
    Evaluate();
  }
  template <typename Most> T Most(Most most) {
    auto sink = new MostSink<T, Most>(std::move(most));
    sinks_.push_back(sink);
    Evaluate();
    return std::move(sink->val());
  }
  template <typename Func> std::pair<bool, T> FindFirst(Func func) {
    auto sink = new FindFirstSink<T, Func>(std::move(func));
    sinks_.push_back(sink);
    Evaluate();
    return {sink->Cancelled(), std::move(sink->val())};
  }

private:
  void Evaluate() {
    for (size_t i = 0; i + 1 < sinks_.size(); ++i) {
      sinks_[i]->set_next(sinks_[i + 1]);
    }
    sinks_[0]->Evaluate(range_);
  }

  Range<T> *range_;
  std::vector<Sink<T> *> sinks_;
};

template <typename T> Stream<T> Range<T>::Stream() { return {this}; }

#endif // TOYS_STREAM_STREAM_H
