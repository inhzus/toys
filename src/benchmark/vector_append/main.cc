//
// Copyright [2020] <inhzus>
//

#include <chrono>
#include <iostream>
#include <vector>

int main() {
  using namespace std::chrono;
  const int kLoop = 1'000;
  const int kCount = 1'000'000;
  {
    auto start = high_resolution_clock::now();
    for (int j = 0; j < kLoop; ++j) {
      std::vector<int> v;
      v.reserve(kCount);
      for (int i = 0; i < kCount; ++i) {
        v.emplace(v.end(), i);
      }
    }
    auto lapse = high_resolution_clock::now() - start;
    std::cout << duration_cast<milliseconds>(lapse).count() << std::endl;
  }
  {
    auto start = high_resolution_clock::now();
    for (int j = 0; j < kLoop; ++j) {
      std::vector<int> v;
      v.resize(kCount);
      for (int i = 0; i < kCount; ++i) {
        v[i] = i;
      }
    }
    auto lapse = high_resolution_clock::now() - start;
    std::cout << duration_cast<milliseconds>(lapse).count() << std::endl;
    // consumes nearly one seconds time compared to the former
  }
  return 0;
}
