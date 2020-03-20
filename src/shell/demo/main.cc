//
// Copyright [2020] <inhzus>
//
// TODO: builtin cd, ls, pwd
//       <C-z>, <C-c>

#include <csetjmp>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

static jmp_buf env;

void sigint_handler(int signo) { _exit(SIGINT); }

int main() {
  signal(SIGINT, sigint_handler);
  std::string raw;
  std::vector<std::string> arg_strs;
  std::vector<char *> args;
  while (true) {
    raw.clear();
    arg_strs.clear();
    args.clear();
    printf("$ ");
    if (!std::getline(std::cin, raw))
      break;
    raw.push_back(' '); // for convenience to get args
    for (size_t i = 0;;) {
      size_t cur = raw.find_first_not_of(' ', i);
      if (cur == std::string::npos)
        break;
      i = raw.find_first_of(' ', cur);
      arg_strs.emplace_back(&raw[cur], &raw[i]);
    }
    args.reserve(arg_strs.size());
    for (std::string &s : arg_strs) {
      args.push_back(&s[0]);
    }
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      exit(EXIT_FAILURE);
    }
    if (pid == 0) {
      int s = execvp(args[0], args.data());
      if (s < 0) {
        perror("exec");
      }
      exit(EXIT_SUCCESS);
    } else {
      int status;
      pid_t wait_pid = waitpid(pid, &status, WUNTRACED);
      if (wait_pid < 0) {
        perror("waitpid");
        exit(EXIT_FAILURE);
      }
    }
  }
}
