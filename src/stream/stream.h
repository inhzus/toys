#ifndef TOYS_STREAM_STREAM_H
#define TOYS_STREAM_STREAM_H

#include <functional>
#include <vector>

#include "range.h"
#include "sink.h"

template <typename T, template <typename... U> typename R> class Stream {
public:
  template <typename... Args> Stream(Args... args) : range_(new R(args...)) {
    sinks_.push_back(new HeadSink<T>());
  }
  ~Stream() {
    delete range_;
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
  std::vector<T> Collect() {
    auto sink = new CollectSink<T>();
    sinks_.push_back(sink);
    Evaluate();
    std::vector<T> vals(std::move(sink->vals()));
    return vals;
  }
  template <typename Func> std::pair<bool, T> FindFirst(Func func) {
    auto sink = new FindFirstSink<T, Func>(std::move(func));
    sinks_.push_back(sink);
    Evaluate();
    return {sink->Cancelled(), std::move(sink->val())};
  }

private:
  void Evaluate() {
    if (dynamic_cast<FinalSink<T> *>(sinks_.back()) == nullptr)
      return;
    for (size_t i = 0; i + 1 < sinks_.size(); ++i) {
      sinks_[i]->set_next(sinks_[i + 1]);
    }
    auto head = sinks_[0];
    head->Pre(range_->Size());
    while (range_->Valid()) {
      if (head->Cancelled())
        break;
      head->Accept(range_->Next());
    }
    head->Post();
  }

  Range<T> *range_;
  std::vector<Sink<T> *> sinks_;
};

#endif // TOYS_STREAM_STREAM_H
