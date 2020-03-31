//
// Copyright [2020] <inhzus>
//

#include "stream.h"

int main() {
  std::vector<int> v{1, 2, 3, 4, 5};
  auto range = BasicRange(v.begin(), v.end());
  auto out = Stream<int>(&range)
                 .Map([](const int &val) { return val * val; })
                 .Filter([](const int &val) { return val % 2 == 1; })
                 .Collect();
  for (auto i : out) {
    printf("%d\n", i);
  }
  return 0;
}

