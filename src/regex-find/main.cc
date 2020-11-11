// Copyright [2020] <inhzus>
#include <fmt/color.h>
#include <fmt/core.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <regex>
#include <stack>
#include <vector>

using std::function;
using std::pair;
using std::regex;
using std::regex_match;
using std::regex_search;
using std::smatch;
using std::stack;
using std::string;
using std::string_view;
using std::vector;
using std::filesystem::current_path;
using std::filesystem::directory_iterator;
using std::filesystem::is_directory;
using std::filesystem::path;
using std::filesystem::relative;

namespace regex_const = std::regex_constants;
struct Options {
  bool case_sensitive;
  bool full_match;
  bool ignore_hidden;
  bool include_directories;
  vector<char *> excludes;
};

class Finder {
 private:
  bool IsExcluded(std::string_view filename) {
    return std::find(options_.excludes.begin(), options_.excludes.end(),
                     filename) != options_.excludes.end();
  }
  bool IsHidden(std::string_view filename) { return filename[0] == '.'; }

 public:
  explicit Finder(const string &s, Options &&options)
      : pattern_{s, options.case_sensitive ? regex_const::ECMAScript
                                           : regex_const::icase},
        options_{std::move(options)} {
    if (options_.full_match) {
      matcher_ = [this](const string &s, smatch *match) {
        return regex_match(s, *match, this->pattern_);
      };
    } else {
      matcher_ = [this](const string &s, smatch *match) {
        return regex_search(s, *match, this->pattern_);
      };
    }
  }
  void parse(const path &p) {
    stack<path> pathes;
    pathes.push(p);
    while (!pathes.empty()) {
      path cur = pathes.top();
      pathes.pop();

      for (auto &leaf : directory_iterator(cur)) {
        path leaf_path{leaf.path()};
        string leaf_name{leaf_path}, filename{leaf_path.filename()};
        if (IsHidden(filename) || IsExcluded(filename)) {
          continue;
        }
        if (leaf.is_directory()) {
          pathes.push(std::move(leaf_path));
          if (!options_.include_directories) {
            continue;
          }
        }
        smatch match;
        if (!matcher_(filename, &match)) {
          continue;
        }
        auto parent_path_len = leaf_name.size() - filename.size();
        string_view parent_name{
            string_view{leaf_name}.substr(0, parent_path_len)};
        fmt::print("{}{}{}{}\n", parent_name, match.prefix().str(),
                   fmt::format(fmt::fg(fmt::color::light_green), match.str()),
                   match.suffix().str());
      }
    }
  }

 private:
  regex pattern_;
  Options options_;
  function<bool(const string &, smatch *)> matcher_;
};

class InputParser {
 public:
  explicit InputParser(int argc, char **argv)
      : beg_(argv + 1), end_(argv + argc) {}
  char *Get(std::string_view key) {
    char **it = std::find(beg_, end_, key);
    if (it != end_ && ++it != end_) {
      return *it;
    }
    return nullptr;
  }

  vector<char *> GetAll(string_view key) {
    vector<char *> res;
    std::copy_if(beg_, end_, std::back_inserter(res),
                 [&key = key](char *p) { return key == p; });
    return res;
  }

  bool Contains(std::string_view key) {
    return std::find(beg_, end_, key) != end_;
  }

 private:
  char **beg_;
  char **end_;
};

int main(int argc, char **argv) {
  InputParser parser(argc, argv);
  if (argc < 2 || parser.Contains("-h")) {
    fmt::print(
        "rf 1.0.0\n"
        "Inhzus <inhzus@gmail.com>\n\n"
        "Regex Find (rf) recursively find files which name matches\n"
        "the provided pattern & in the provided directory which is\n"
        "by default the current directory.\n\n"
        "USAGE:\n"
        "    rf [OPTIONS] <PATTERN>\n\n"
        "OPTIONS:\n"
        "    -c     Case sensitive\n"
        "    -d     Include results which are directories\n"
        "    -f     Full match\n"
        "    -h     Prints help information\n"
        "    -i     Ignores hidden files and directories\n"
        "    -p     Root path\n"
        "    -x     Can be used multiple times to exclude directories\n"
        "           or files.\n");
    return argc < 2;
  }
  Options options;
  options.case_sensitive = parser.Contains("-c");
  options.include_directories = parser.Contains("-d");
  options.full_match = parser.Contains("-f");
  options.ignore_hidden = parser.Contains("-i");
  options.excludes = parser.GetAll("-x");
  char *arg_path = parser.Get("-p");
  path root_path(arg_path == nullptr ? current_path() : arg_path);
  Finder finder(argv[argc - 1], std::move(options));
  finder.parse(relative(root_path));
  return 0;
}
bool flag;
