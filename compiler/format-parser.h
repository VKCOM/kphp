// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "inferring/type-data.h"

class FormatString final {
private:
  enum class State {
    Text,
    StartSpecifier,
    StartPlaceholderSpecifier,
    SpecifierBody,
    EndSpecifier
  };

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

    bool is_simple() const {
      if (width != 0 || precision != 0 || with_precision || show_sign || pad_right || placeholder != 0) {
        return false;
      }

      switch (type) {
        case SpecifierType::Decimal:
        case SpecifierType::String:
          return true;
        case SpecifierType::Float:
        case SpecifierType::Binary:
        case SpecifierType::Octal:
        case SpecifierType::Hexadecimal:
        case SpecifierType::Unsigned:
        case SpecifierType::Char:
        default:
          return false;
      }

      return false;
    }

    std::string expected_type() const {
      switch (type) {
        case SpecifierType::Unknown:
        case SpecifierType::Incomplete:
          return "";

        case SpecifierType::Binary:
        case SpecifierType::Decimal:
        case SpecifierType::Octal:
        case SpecifierType::Hexadecimal:
        case SpecifierType::Unsigned:
        case SpecifierType::Char:
          return "int";

        case SpecifierType::Float:
          return "float";

        case SpecifierType::String:
          return "string";
      }

      return "";
    }

    bool allowed_for(const TypeData *typ) const {
      if (typ->ptype() == tp_mixed) {
        return true;
      }

      if (typ->or_null_flag() || typ->or_false_flag()) {
        return false;
      }

      switch (type) {
        case SpecifierType::Unknown:
        case SpecifierType::Incomplete:
          return false;

        case SpecifierType::Binary:
        case SpecifierType::Decimal:
        case SpecifierType::Octal:
        case SpecifierType::Hexadecimal:
        case SpecifierType::Unsigned:
        case SpecifierType::Char:
        case SpecifierType::Float:
        case SpecifierType::String:
          return vk::any_of_equal(typ->ptype(), tp_int, tp_float, tp_string);
      }

      return false;
    }

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
    std::string format;
    std::vector<Part> parts;
    std::vector<std::string> errors;

    int min_count() const {
      int min = 0;
      int min_with_number = 0;

      for (const auto &part : parts) {
        if (!part.is_specifier()) {
          continue;
        }

        if (part.specifier.arg_num == -1) {
          min++;
        } else if (part.specifier.arg_num > min_with_number) {
          min_with_number = part.specifier.arg_num;
        }
      }

      return std::max(min, min_with_number);
    }

    std::vector<Part> get_by_index(size_t index) const {
      std::vector<Part> res;
      size_t index_spec_without_num = 0;

      for (const auto &part : parts) {
        if (!part.is_specifier()) {
          continue;
        }

        if (part.specifier.arg_num == -1) {
          index_spec_without_num++;

          if (index_spec_without_num == index) {
            res.push_back(part);
          }
        }

        if (part.specifier.arg_num == index) {
          res.push_back(part);
          continue;
        }
      }

      return res;
    }
  };

  static ParseResult parse_format(const std::string &format) {
    std::vector<Part> parts;
    std::vector<std::string> errors;
    std::string last_value;
    Specifier last_spec = Specifier{SpecifierType::Incomplete};
    State state = State::Text;

    for (int symbol_index = 0; symbol_index < format.size(); ++symbol_index) {
      const auto &symbol = format[symbol_index];

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
            if (last_spec.width != 0) {
              errors.emplace_back("The width value is ignored for the '%c' specifier");
            }
            if (last_spec.placeholder != 0) {
              errors.emplace_back("The padding value is ignored for the '%c' specifier");
            }

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
            check_if_incomplete_spec(state, parts, errors, last_spec);
            last_value += " ";
            break;
          case '$':
            state = State::StartSpecifier;

            if (last_spec.placeholder != 0 && last_spec.placeholder != '0') {
              errors.emplace_back("The argument number must come before the padding symbol");
            }

            last_spec.arg_num = last_spec.width;
            last_spec.width = 0;

            if (last_spec.placeholder == '0') {
              last_spec.placeholder = 0;
            }

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
                if (last_spec.width == 0 && symbol == '0' && last_spec.placeholder == 0) {
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

        symbol_index--;
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

    check_if_incomplete_spec(state, parts, errors, last_spec);

    return ParseResult{format, parts, errors};
  }

  static void check_if_incomplete_spec(State state, std::vector<Part> &parts, std::vector<std::string> &errors, Specifier &last_spec) {
    if (last_spec.type != SpecifierType::Incomplete) {
      return;
    }

    if (last_spec.placeholder != 0 || last_spec.arg_num != -1 || last_spec.width != 0 || state == State::StartSpecifier) {
      parts.push_back(Part{last_spec.to_string()});
      errors.push_back(std::string("Incomplete type specifier '") + parts.back().value + "'");
      last_spec = Specifier{};
    }
  }
};
