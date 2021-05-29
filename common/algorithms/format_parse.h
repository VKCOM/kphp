#pragma once

class FormatString final {
private:
  enum class State { Text, StartSpecifier, StartPlaceholderSpecifier, SpecifierBody, EndSpecifier };

public:
  enum class SpecifierType {
    Unknown,
    Incomplete,
    Binary,
    Decimal,
    Octal,
    Hexadecimal,
    Unsigned,
    Char,
    Float,
    String,
  };

  struct Specifier {
    SpecifierType type{SpecifierType::Unknown};
    char fact_symbol{0};
    size_t width{0};
    size_t precision{0};
    size_t arg_num{static_cast<size_t>(-1)};

    bool with_precision{false};
    bool show_sign{false};
    bool pad_right{false};
    char placeholder{0};

    std::string to_string() const {
      std::string res;

      res += "%";
      if (arg_num != -1) {
        res += std::to_string(arg_num) + '$';
      }
      if (show_sign) {
        res += "+";
      }
      if (pad_right) {
        res += "-";
      }
      if (placeholder != 0) {
        if (placeholder == '0') {
          res += placeholder;
        } else {
          res += '\'';
          res += placeholder;
        }
      }
      if (width != 0) {
        res += std::to_string(width);
      }
      if (with_precision) {
        res += ".";
        res += std::to_string(precision);
      }

      if (fact_symbol != 0) {
        res += fact_symbol;
      }

      return res;
    }
  };

  struct Part {
    std::string value;
    Specifier specifier;

    bool is_specifier() const {
      return value.empty();
    }

    std::string to_string() const {
      if (!value.empty()) {
        return value;
      }

      return specifier.to_string();
    }
  };

  struct ParseResult {
    std::vector<Part> parts;
    std::vector<std::string> errors;

    bool ok() const {
      return errors.empty();
    }
  };

  static ParseResult parse_format(const std::string &format) {
    std::vector<Part> parts;
    std::vector<std::string> errors;
    std::string last_value;
    Specifier last_spec = Specifier{SpecifierType::Incomplete};
    State state = State::Text;

    for (const auto &symbol : format) {
      if (state == State::Text && symbol == '%') {
        state = State::StartSpecifier;

        if (!last_value.empty()) {
          parts.push_back(Part{last_value});
          last_value = "";
        }
        continue;
      }

      if (state == State::StartSpecifier) {
        if (symbol == '\'') {
          state = State::StartPlaceholderSpecifier;
          continue;
        } else if (symbol == ' ') {
          continue;
        }
        state = State::SpecifierBody;
      }

      if (state == State::StartPlaceholderSpecifier) {
        last_spec.placeholder = symbol;
        state = State::SpecifierBody;
        continue;
      }

      if (state == State::SpecifierBody) {
        state = State::EndSpecifier;

        switch (symbol) {
          case 'b':
            last_spec.type = SpecifierType::Binary;
            last_spec.fact_symbol = symbol;
            break;
          case 'd':
            last_spec.type = SpecifierType::Decimal;
            last_spec.fact_symbol = symbol;
            break;
          case 'u':
            last_spec.type = SpecifierType::Unsigned;
            last_spec.fact_symbol = symbol;
            break;
          case 'o':
            last_spec.type = SpecifierType::Octal;
            last_spec.fact_symbol = symbol;
            break;
          case 'x':
          case 'X':
            last_spec.type = SpecifierType::Hexadecimal;
            last_spec.fact_symbol = symbol;
            break;
          case 'e':
          case 'E':
          case 'f':
          case 'F':
          case 'g':
          case 'G':
            last_spec.type = SpecifierType::Float;
            last_spec.fact_symbol = symbol;
            break;
          case 'c':
            last_spec.type = SpecifierType::Char;
            last_spec.fact_symbol = symbol;
            break;
          case 's':
            if (last_spec.show_sign) {
              errors.emplace_back("The '+' flag with the %s modifier is meaningless");
            }

            last_spec.type = SpecifierType::String;
            last_spec.fact_symbol = symbol;
            break;
          case '%':
            state = State::Text;
            parts.push_back(Part{"%%"});
            break;
          case ' ':
            check_if_incomplete_spec(parts, errors, last_spec);
            last_value += " ";
            break;
          case '$':
            state = State::SpecifierBody;

            last_spec.arg_num = last_spec.width;
            last_spec.width = 0;
            last_spec.placeholder = 0;

            if (last_spec.arg_num == 0) {
              errors.emplace_back("The argument number must be greater than zero");
            }
            break;
          default:
            state = State::SpecifierBody;

            if (symbol >= '0' && symbol <= '9') {
              const auto number = symbol - '0';
              if (last_spec.with_precision) {
                last_spec.precision = last_spec.precision * 10 + number;
              } else {
                if (last_spec.width == 0 && symbol == '0') {
                  last_spec.placeholder = '0';
                } else {
                  last_spec.width = last_spec.width * 10 + number;
                }
              }
            } else if (symbol == '+') {
              last_spec.show_sign = true;
            } else if (symbol == '-') {
              last_spec.pad_right = true;
            } else if (symbol == '.') {
              last_spec.with_precision = true;
            } else {
              errors.emplace_back(std::string("Unsupported specifier '%") + symbol + "'");
              last_value = last_spec.to_string() + symbol;
              last_spec.type = SpecifierType::Unknown;
              state = State::Text;
            }

            break;
        }
        continue;
      } else if (state == State::EndSpecifier) {
        state = State::Text;
        parts.push_back(Part{"", last_spec});
        last_spec = Specifier{};

        last_value += symbol;
        continue;
      }

      last_value += symbol;
    }

    if (!last_value.empty()) {
      parts.push_back(Part{last_value});
    }
    if (state == State::EndSpecifier) {
      parts.push_back(Part{"", last_spec});
    }

    check_if_incomplete_spec(parts, errors, last_spec);

    return ParseResult{parts, errors};
  }

  static void check_if_incomplete_spec(std::vector<Part> &parts, std::vector<std::string> &errors, Specifier &last_spec) {
    if (last_spec.type != SpecifierType::Incomplete) {
      return;
    }

    if (last_spec.placeholder != 0 || last_spec.arg_num != -1 || last_spec.width != 0) {
      parts.push_back(Part{last_spec.to_string()});
      errors.push_back(std::string("Incomplete type specifier \"") + parts.back().value + "\"");
      last_spec = Specifier{};
    }
  }
};
