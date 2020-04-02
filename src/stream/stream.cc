//
// Copyright [2020] <inhzus>
//

#include "stream.h"

int main() {
  auto out = StepRange(0, 10, 1)
                 .Stream()
                 .Map([](int val) { return val * val; })
                 .Filter([](int val) { return val % 2 == 1; })
                 .Collect();
  for (auto i : out) {
    printf("%d\n", i);
  }
  auto [found, val] =
      StepRange(0, 30, 1).Stream().FindFirst([](int val) { return val > 20; });
  printf("%d\n", val);

  auto sorted = StepRange(25, 4, -7)
                    .Stream()
                    .Sort([](int lhs, int rhs) { return lhs % 17 < rhs % 17; })
                    .Collect();
  for (auto i : sorted) {
    printf("%d\n", i);
  }
  auto max = BasicRange(sorted.begin(), sorted.end())
                 .Stream()
                 .Most([](int lhs, int rhs) { return lhs < rhs ? rhs : lhs; });
  printf("%d\n", max);
  auto r = StepRange(1, 10, 1).Stream();
  return 0;
}

