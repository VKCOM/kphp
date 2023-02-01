// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/exec.h"

#include <cstdlib>
#include <functional>

#include "common/smart_ptrs/unique_ptr_with_delete_function.h"

static size_t strip_trailing_whitespace(char *buffer, int bytes_read) {
  size_t l = bytes_read;
  while (l-- > 0 && isspace((reinterpret_cast<unsigned char *>(buffer))[l])){};
  if (l != (bytes_read - 1)) {
    bytes_read = l + 1;
    buffer[bytes_read] = '\0';
  }
  return bytes_read;
}

static void pclose_wrapper(FILE *f) {
  pclose(f);
}

struct ExecStatus {
  bool success{false};
  int exit_code{0};
  string last_line;
};

using ExecHandler = std::function<size_t(char *, std::size_t)>;

static ExecStatus exec_impl(const string &cmd, const ExecHandler &handler) {
  if (cmd.empty()) {
    php_warning("Cannot execute a blank command");
    return {};
  }

  vk::unique_ptr_with_delete_function<FILE, pclose_wrapper> fp{popen(cmd.c_str(), "r")};
  if (!fp) {
    php_warning("Unable to fork [%s]", cmd.c_str());
    return {};
  }

  char *line = nullptr;
  std::size_t line_size = 0;
  int bytes_read = 0;
  for (;;) {
    const int res = getline(&line, &line_size, fp.get());
    if (res == -1) {
      break;
    }
    bytes_read = handler(line, res);
  }

  string last_line;
  if (bytes_read) {
    bytes_read = strip_trailing_whitespace(line, bytes_read);
    last_line.assign(line, bytes_read);
  }
  free(line);

  int ret = pclose(fp.release());
  if (WIFEXITED(ret)) {
    ret = WEXITSTATUS(ret);
  }
  return {true, ret, last_line};
}

static ExecStatus passthru_impl(const string &cmd) {
  if (cmd.empty()) {
    php_warning("Cannot execute a blank command");
    return {};
  }

  vk::unique_ptr_with_delete_function<FILE, pclose_wrapper> fp{popen(cmd.c_str(), "r")};
  if (!fp) {
    php_warning("Unable to fork [%s]", cmd.c_str());
    return {};
  }

  std::array<char, 4096> BUFF = {};
  std::size_t bytes_read = 0;
  const int fd = fileno(fp.get());
  while ((bytes_read = read(fd, BUFF.data(),  BUFF.size())) > 0) {
    write(STDOUT_FILENO, BUFF.data(), bytes_read);
    [[maybe_unused]] auto res = fflush(stdout);
  }

  int ret = pclose(fp.release());
  if (WIFEXITED(ret)) {
    ret = WEXITSTATUS(ret);
  }
  return {true, ret, {}};
}

int64_t &get_dummy_result_code() noexcept {
  static int64_t result_code = 0;
  return result_code;
}

Optional<string> f$exec(const string &command) {
  auto [success, _, last_line] = exec_impl(command, [](char */*buff*/, std::size_t size) { return size; });
  return success ? Optional<string>{last_line} : Optional<string>{false};
}

Optional<string> f$exec(const string &command, mixed &output, int64_t &result_code) {
  if (!output.is_array()) {
    output = array<mixed>();
  }
  auto [success, result, last_line] = exec_impl(command, [&output](char *buff, std::size_t size) {
    const std::size_t bytes_read = strip_trailing_whitespace(buff, size);
    output.as_array().push_back(string(buff, bytes_read));
    return bytes_read;
  });

  if (success) {
    result_code = result;
    return last_line;
  }
  return false;
}

Optional<string> f$system(const string &command, int64_t &result_code) {
  auto [success, result, last_line] = exec_impl(command, [](char *buff, std::size_t size) {
    write(STDOUT_FILENO, buff, size);
    [[maybe_unused]] auto res = fflush(stdout);
    return size;
  });

  if (success) {
    result_code = result;
    return last_line;
  }
  return false;
}

Optional<bool> f$passthru(const string &command, int64_t &result_code) {
  auto [success, result, _] = passthru_impl(command);
  if (success) {
    result_code = result;
    return {};
  }
  return false;
}

string f$escapeshellarg(const string &arg) noexcept {
  php_assert(std::strlen(arg.c_str()) == arg.size() && "Input string contains NULL bytes");

  string result;
  result.reserve_at_least(arg.size() + 2);
  result.push_back('\'');

  for (std::size_t i = 0; i < arg.size(); ++i) {
    if (arg[i] == '\'') {
      result.push_back('\'');
      result.push_back('\\');
      result.push_back('\'');
    }
    result.push_back(arg[i]);
  }

  result.push_back('\'');
  return result;
}

string f$escapeshellcmd(const string &cmd) noexcept {
  php_assert(std::strlen(cmd.c_str()) == cmd.size() && "Input string contains NULL bytes");

  string result;
  result.reserve_at_least(cmd.size());

  const char *p = nullptr;

  for (std::size_t i = 0; i < cmd.size(); ++i) {
    switch (cmd[i]) {
      case '"':
      case '\'':
        if (!p && (p = static_cast<const char *>(std::memchr(cmd.c_str() + i + 1, cmd[i], cmd.size() - i - 1)))) {
          // noop
        } else if (p && *p == cmd[i]) {
          p = nullptr;
        } else {
          result.push_back('\\');
        }
        result.push_back(cmd[i]);
        break;
      case '#': // this is character-set independent
      case '&':
      case ';':
      case '`':
      case '|':
      case '*':
      case '?':
      case '~':
      case '<':
      case '>':
      case '^':
      case '(':
      case ')':
      case '[':
      case ']':
      case '{':
      case '}':
      case '$':
      case '\\':
      case '\x0A':
      case '\xFF':
        result.push_back('\\');
        [[fallthrough]];
      default:
        result.push_back(cmd[i]);
    }
  }

  return result;
}
