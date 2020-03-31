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
  virtual ~Stream() = default;
  Stream &Map(const std::function<T(const T &)> &func) {
    sinks_.push_back(new MapSink<T>(func));
    return *this;
  }
  Stream &Filter(const std::function<bool(const T &)> &func) {
    sinks_.push_back(new FilterSink<T>(func));
    return *this;
  }
  std::vector<T> Collect() {
    auto sink = new CollectSink<T>();
    sinks_.push_back(sink);
    Evaluate();
    std::vector<T> vals(std::move(sink->vals()));
    return vals;
  }

private:
  void Evaluate() {
    if (dynamic_cast<FinalSink<T> *>(sinks_.back()) == nullptr)
      return;
    for (size_t i = 0; i + 1 < sinks_.size(); ++i) {
      sinks_[i]->set_next(sinks_[i + 1]);
    }
    sinks_[0]->Pre(range_->Size());
    while (range_->Valid()) {
      sinks_[0]->Accept(range_->Next());
    }
    sinks_[0]->Post();
  }

  Range<T> *range_;
  std::vector<Sink<T> *> sinks_;
};

#endif // TOYS_STREAM_STREAM_H
