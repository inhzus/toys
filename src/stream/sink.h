#ifndef TOYS_STREAM_SINK_H
#define TOYS_STREAM_SINK_H

#include <cassert>
#include <cstddef>
#include <functional>
#include <vector>

template <typename T> class Sink {
public:
  Sink() : next_(nullptr) {}
  virtual ~Sink() = default;
  virtual void Pre(size_t len) = 0;
  virtual void Accept(const T &val) = 0;
  virtual void Post() = 0;
  [[nodiscard]] virtual bool cancelled() const { return false; };
  void set_next(Sink *next) { next_ = next; }

protected:
  Sink<T> *next_;
};

template <typename T> class BasicSink : public Sink<T> {
public:
  void Pre(size_t len) final {
    assert(this->next_ != nullptr);
    DoPre(len);
  }
  void Accept(const T &val) final {
    assert(this->next_ != nullptr);
    DoAccept(val);
  }
  void Post() final {
    assert(this->next_ != nullptr);
    DoPost();
  }

protected:
  virtual void DoPre(size_t len) = 0;
  virtual void DoAccept(const T &val) = 0;
  virtual void DoPost() = 0;
};

template <typename T> class HeadSink : public BasicSink<T> {
private:
  void DoPre(size_t len) final { this->next_->Pre(len); }
  void DoAccept(const T &val) final { this->next_->Accept(val); }
  void DoPost() final { this->next_->Post(); }
};

template <typename T> class MapSink : public BasicSink<T> {
public:
  MapSink(std::function<T(const T &)> func) : BasicSink<T>(), func_(func) {}

private:
  void DoPre(size_t len) final { this->next_->Pre(len); }
  void DoAccept(const T &val) final { this->next_->Accept(func_(val)); }
  void DoPost() final { this->next_->Post(); }

  std::function<T(const T &)> func_;
};

template <typename T> class FilterSink : public BasicSink<T> {
public:
  FilterSink(std::function<bool(const T &)> func)
      : BasicSink<T>(), func_(func) {}

private:
  void DoPre(size_t len) final { this->next_->Pre(len); }
  void DoAccept(const T &val) final {
    if (func_(val)) {
      this->next_->Accept(val);
    }
  }
  void DoPost() final { this->next_->Post(); }

  std::function<bool(const T &)> func_;
};

template <typename T> class FinalSink : public Sink<T> {};

template <typename T> class BreakableSink : public FinalSink<T> {
public:
  BreakableSink() : Sink<T>(), cancelled_(false) {}
  [[nodiscard]] bool cancelled() const { return cancelled_; }

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

#endif // TOYS_STREAM_SINK_H
