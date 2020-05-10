#include <cstdio>

class Foo {
public:
  Foo() { printf("constructor\n"); }
  Foo(const Foo &) { printf("assignment constructor\n"); }
  Foo(Foo &&) { printf("move constructor\n"); }
  Foo &operator=(const Foo &) {
    printf("assignment operator\n");
    return *this;
  }
  Foo &operator=(Foo &&) {
    printf("move operator\n");
    return *this;
  }
};

Foo go() {
  Foo foo;
  return foo;
}

int main() { Foo foo = go(); }
