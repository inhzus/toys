//
// Copyright [2020] <inhzus>
//
// simulate linux pipe
// run "ls / | wc -l" as "./a.out - ls / - wc -l"

#include <cstdio>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

void pipe_run(char **lhs, char **rhs) {
  int fd[2];
  pipe(fd);
  pid_t pid = fork();
  if (pid == -1) {
    perror("pid");
    _exit(1);
  }
  if (pid == 0) {
    dup2(fd[0], STDIN_FILENO);
    close(fd[1]);
    execvp(rhs[0], rhs);
    perror("exec rhs");
    _exit(1);
  } else {
    dup2(fd[1], STDOUT_FILENO);
    close(fd[0]);
    execvp(lhs[0], lhs);
    perror("exec lhs");
    _exit(1);
  }
}

int main(int argc, char **argv) {
  char *raws[2];
  int left_start = 0, right_start = 0;
  for (int i = 0; i < argc; ++i) {
    if (strcmp(argv[i], "-"))
      continue;
    if (left_start == 0) {
      left_start = i + 1;
    } else {
      right_start = i + 1;
      break;
    }
  }
  std::vector<char *> lhs, rhs;
  lhs.reserve(right_start - left_start - 1);
  rhs.reserve(argc - right_start);
  for (int i = left_start; i + 1 != right_start; ++i) {
    lhs.push_back(argv[i]);
  }
  lhs.push_back(nullptr);
  for (int i = right_start; i != argc; ++i) {
    rhs.push_back(argv[i]);
  }
  rhs.push_back(nullptr);
  pipe_run(lhs.data(), rhs.data());
  return 0;
}

