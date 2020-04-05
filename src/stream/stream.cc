//
// Copyright [2020] <inhzus>
//

#include "stream.h"

int main() {
  Stream(StepRange(0, 10, 1))
      .Map([](auto val) { return val * val; })
      .Filter([](auto val) { return val % 2 == 1; })
      .ForEach([](auto val) { printf("%d ", val); });
  printf("\n---\n");  // 1 9 25 49 81
  auto [found, val] =
      Stream(StepRange(0, 30, 1)).FindFirst([](int val) { return val > 20; });
  printf("%d\n---\n", val);  // 21
  auto sorted = Stream(StepRange(25, 4, [](auto *val) { *val -= 7; }))
                    .Sort([](int lhs, int rhs) { return lhs % 17 < rhs % 17; })
                    .Collect();
  for (auto i : sorted) {
    printf("%d ", i);
  }
  printf("\n---\n");  // 18 25 11

  auto max =
      Stream(&sorted).Most([](int lhs, int rhs) { return std::max(lhs, rhs); });
  printf("%d\n---\n", max);  // 25
  auto max1 = Stream(StepRange(
                         0, [](auto val) { return val < 17; }, 5))
                  .Most([](auto lhs, auto rhs) { return std::max(lhs, rhs); });
  printf("%d\n---\n", max1);  // 15
  auto max2 = Stream(StepRange(
                         1, [](auto val) { return val < 17; },
                         [](auto *val) { *val *= 2; }))
                  .Most([](auto lhs, auto rhs) { return std::max(lhs, rhs); });
  printf("%d\n---\n", max2);  // 16

  Stream(StepRange(0, 10, 1))
      .Map([](int val) { return std::to_string(val); })
      .Map([](const std::string &s) { return std::stoi(s); })
      .ForEach([](auto val) { printf("%d ", val); });
  printf("\n---\n");  // 0 1 2 3 4 5 6 7 8 9
  auto [_1, five] = Stream(StepRange(0, 10, 1))
                        .Map([](int val) { return std::to_string(val); })
                        .Map([](const std::string &s) { return std::stoi(s); })
                        .FindFirst([](auto val) { return val > 4; });
  printf("%d\n---\n", five);  // 5

  Stream(StepRange(0, 12, 3))
      .FlatMap([](int val) {
        return std::vector({val, val + 1, val + 2});
      })
      .ForEach([](int val) { printf("%d ", val); });
  printf("\n---\n");  // 0 1 2 3 4 5 6 7 8 9 10 11

  std::vector strs{std::string("hello"), std::string("world")};
  Stream(&strs).FlatMap([](const auto &s) { return s; }).ForEach([](char c) {
    printf("%c ", c);
  });
  printf("\n---\n");  // h e l l o w o r l d
  return 0;
}
