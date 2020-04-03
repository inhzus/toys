//
// Copyright [2020] <inhzus>
//

#include "stream.h"

int main() {
  Stream(StepRange(0, 10, 1))
      .Map([](auto val) { return val * val; })
      .Filter([](auto val) { return val % 2 == 1; })
      .ForEach([](auto val) { printf("%d ", val); });
  printf("\n---\n");
  auto [found, val] =
      Stream(StepRange(0, 30, 1)).FindFirst([](int val) { return val > 20; });
  printf("%d\n---\n", val);
  auto sorted = Stream(StepRange(25, 4, [](auto *val) { *val -= 7; }))
                    .Sort([](int lhs, int rhs) { return lhs % 17 < rhs % 17; })
                    .Collect();
  for (auto i : sorted) {
    printf("%d ", i);
  }
  printf("\n---\n");

  auto max =
      Stream(&sorted).Most([](int lhs, int rhs) { return std::max(lhs, rhs); });
  printf("%d\n---\n", max);
  auto max1 = Stream(StepRange(
                         0, [](auto val) { return val < 17; }, 5))
                  .Most([](auto lhs, auto rhs) { return std::max(lhs, rhs); });
  printf("%d\n---\n", max1);
  auto max2 = Stream(StepRange(
                         1, [](auto val) { return val < 17; },
                         [](auto *val) { *val *= 2; }))
                  .Most([](auto lhs, auto rhs) { return std::max(lhs, rhs); });
  printf("%d\n---\n", max2);
  return 0;
}
