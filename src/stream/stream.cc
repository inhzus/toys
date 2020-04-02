//
// Copyright [2020] <inhzus>
//

#include "stream.h"

int main() {
  StepRange(0, 10, 1)
      .Stream()
      .Map([](int val) { return val * val; })
      .Filter([](int val) { return val % 2 == 1; })
      .ForEach([](int val) { printf("%d ", val); });
  printf("\n---\n");

  auto [found, val] =
      StepRange(0, 30, 1).Stream().FindFirst([](int val) { return val > 20; });
  printf("%d\n", val);
  printf("---\n");

  auto sorted = StepRange(25, 4, -7)
                    .Stream()
                    .Sort([](int lhs, int rhs) { return lhs % 17 < rhs % 17; })
                    .Collect();
  for (auto i : sorted) {
    printf("%d ", i);
  }
  printf("\n---\n");

  auto max = ItRange(sorted.begin(), sorted.end())
                 .Stream()
                 .Most([](int lhs, int rhs) { return std::max(lhs, rhs); });
  printf("%d\n", max);
  printf("---\n");
  return 0;
}
