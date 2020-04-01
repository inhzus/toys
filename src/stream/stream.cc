//
// Copyright [2020] <inhzus>
//

#include "stream.h"

int main() {
  auto out = Stream<int, StepRange>(0, 10, 1)
                 .Map([](int val) { return val * val; })
                 .Filter([](int val) { return val % 2 == 1; })
                 .Collect();
  for (auto i : out) {
    printf("%d\n", i);
  }
  auto [found, val] = Stream<int, StepRange>(0, 30, 1).FindFirst(
      [](int val) { return val > 20; });
  printf("%d\n", val);
  return 0;
}

