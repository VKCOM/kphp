#include <gtest/gtest.h>

#include "compiler/format-parser.h"

#define ADDITION_INFO(index, format) \
  "test case #" << (index) << " (format: " << (format) << ")"

#define ASSERT_EQ_FORMAT(x, y, index, format) \
  ASSERT_EQ(x, y) << ADDITION_INFO(index, format)

#define ASSERT_STREQ_FORMAT(x, y, index, format) \
  ASSERT_STREQ(x, y) << ADDITION_INFO(index, format)

struct TestCase {
  std::string format;
  FormatString::ParseResult result;
};

using PR = FormatString::ParseResult;
using Part = FormatString::Part;
using Spec = FormatString::Specifier;
using SpecType = FormatString::SpecifierType;

TEST(format_parser_test, test_parse) {
  std::vector<TestCase> cases = {
    {
      "% %",
      PR{
        .parts={Part{.value="%%"}},
        .errors={}
      }
    },
    {
      "%%",
      PR{
        .parts={Part{.value="%%"}},
        .errors={}
      }
    },
    {
      "%%%d",
      PR{
        .parts={Part{.value="%%"}, Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d'}}},
        .errors={}
      }
    },
    {
      "%d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d'}}},
        .errors={}
      }
    },
    {
      "%10d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10}}},
        .errors={}
      }
    },
    {
      "%.5d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .precision=5, .with_precision=true}}},
        .errors={}
      }
    },
    {
      "%5.5d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=5, .precision=5, .with_precision=true}}},
        .errors={}
      }
    },
    {
      "%+d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .show_sign=true}}},
        .errors={}
      }
    },
    {
      "%-d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .pad_right=true}}},
        .errors={}
      }
    },
    {
      "%-10d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .pad_right=true}}},
        .errors={}
      }
    },
    {
      "%-1d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=1, .pad_right=true}}},
        .errors={}
      }
    },
    {
      "%-10.5d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .precision=5, .with_precision=true, .pad_right=true}}},
        .errors={}
      }
    },
    {
      "%-10.50d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .precision=50, .with_precision=true, .pad_right=true}}},
        .errors={}
      }
    },
    {
      "%1$d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .arg_num=1}}},
        .errors={}
      }
    },
    {
      "%15$d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .arg_num=15}}},
        .errors={}
      }
    },
    {
      "%010d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .placeholder='0'}}},
        .errors={}
      }
    },
    {
      "% 10d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .placeholder='\0'}}},
        .errors={}
      }
    },
    {
      "%  10d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .placeholder='\0'}}},
        .errors={}
      }
    },
    {
      "%'#10d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .placeholder='#'}}},
        .errors={}
      }
    },
    {
      "%'$10d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .placeholder='$'}}},
        .errors={}
      }
    },
    {
      "%'010d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .placeholder='0'}}},
        .errors={}
      }
    },
    {
      "%1$'#10d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .arg_num=1, .placeholder='#'}}},
        .errors={}
      }
    },
    {
      "%1$'#10.10d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .precision=10, .arg_num=1, .with_precision=true, .placeholder='#'}}},
        .errors={}
      }
    },
    {
      "%1$'#10.d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .precision=0, .arg_num=1, .with_precision=true, .placeholder='#'}}},
        .errors={}
      }
    },
    {
      "%1$'#+10.10d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .precision=10, .arg_num=1, .with_precision=true, .show_sign=true, .placeholder='#'}}},
        .errors={}
      }
    },
    {
      "%1$'#-10.10d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .precision=10, .arg_num=1, .with_precision=true, .pad_right=true, .placeholder='#'}}},
        .errors={}
      }
    },
    {
      "%1$'#+-10.10d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .precision=10, .arg_num=1, .with_precision=true, .show_sign=true, .pad_right=true, .placeholder='#'}}},
        .errors={}
      }
    },
    {
      "%1$'#-+10.10d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .precision=10, .arg_num=1, .with_precision=true, .show_sign=true, .pad_right=true, .placeholder='#'}}},
        .errors={}
      }
    },

    // errors
    {
      "%w",
      PR{
        .parts={Part{.value="%w"}},
        .errors={"Unsupported specifier '%w'"}
      }
    },
    {
      "%#1d",
      PR{
        .parts={Part{.value="%#1d"}},
        .errors={"Unsupported specifier '%#'"}
      }
    },
    {
      "%0$d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .arg_num=0}}},
        .errors={"The argument number must be greater than zero"}
      }
    },
    {
      "%'#0$d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .arg_num=0, .placeholder='#'}}},
        .errors={
          "The argument number must come before the padding symbol",
          "The argument number must be greater than zero"
        }
      }
    },
    {
      "%'#1$d",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .arg_num=1, .placeholder='#'}}},
        .errors={"The argument number must come before the padding symbol"}
      }
    },
    {
      "%+s",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::String, .fact_symbol='s', .show_sign=true}}},
        .errors={"The '+' flag with the %s modifier is meaningless"}
      }
    },
    {
      "%1$'#-+10.10s",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::String, .fact_symbol='s', .width=10, .precision=10, .arg_num=1, .with_precision=true, .show_sign=true, .pad_right=true, .placeholder='#'}}},
        .errors={"The '+' flag with the %s modifier is meaningless"}
      }
    },
    {
      "%10c",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Char, .fact_symbol='c', .width=10}}},
        .errors={"The width value is ignored for the '%c' specifier"}
      }
    },
    {
      "%0c",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Char, .fact_symbol='c', .placeholder='0'}}},
        .errors={"The padding value is ignored for the '%c' specifier"}
      }
    },
    {
      "%'#c",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Char, .fact_symbol='c', .placeholder='#'}}},
        .errors={"The padding value is ignored for the '%c' specifier"}
      }
    },
    {
      "%010c",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Char, .fact_symbol='c', .width=10, .placeholder='0'}}},
        .errors={
          "The width value is ignored for the '%c' specifier",
          "The padding value is ignored for the '%c' specifier"
        }
      }
    },
    {
      "%'#10c",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Char, .fact_symbol='c', .width=10, .placeholder='#'}}},
        .errors={
          "The width value is ignored for the '%c' specifier",
          "The padding value is ignored for the '%c' specifier"
        }
      }
    },
    {
      "%1$'#-+10.10c",
      PR{
        .parts={Part{.specifier=Spec{.type=SpecType::Char, .fact_symbol='c', .width=10, .precision=10, .arg_num=1, .with_precision=true, .show_sign=true, .pad_right=true, .placeholder='#'}}},
        .errors={
          "The width value is ignored for the '%c' specifier",
          "The padding value is ignored for the '%c' specifier"
        }
      }
    },

    // incomplete errors
    {
      "%",
      PR{
        .parts={Part{.value="%"}},
        .errors={"Incomplete type specifier '%'"}
      }
    },
    {
      "%10",
      PR{
        .parts={Part{.value="%10"}},
        .errors={"Incomplete type specifier '%10'"}
      }
    },
    {
      "%1$10",
      PR{
        .parts={Part{.value="%1$10"}},
        .errors={"Incomplete type specifier '%1$10'"}
      }
    },

    // several specifiers
    {
      "%d%d",
      PR{
        .parts={
          Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d'}},
          Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d'}}
        },
        .errors={}
      }
    },
    {
      "%d %s",
      PR{
        .parts={
          Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d'}},
          Part{.value=" "},
          Part{.specifier=Spec{.type=SpecType::String, .fact_symbol='s'}}
        },
        .errors={}
      }
    },
    {
      "%d word %d",
      PR{
        .parts={
          Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d'}},
          Part{.value=" word "},
          Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d'}}
        },
        .errors={}
      }
    },
    {
      "Hello %d word %d",
      PR{
        .parts={
          Part{.value="Hello "},
          Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d'}},
          Part{.value=" word "},
          Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d'}}
        },
        .errors={}
      }
    },
    {
      "Hello %1$'#-+10.10d word %d",
      PR{
        .parts={
          Part{.value="Hello "},
          Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .precision=10, .arg_num=1, .with_precision=true, .show_sign=true, .pad_right=true, .placeholder='#'}},
          Part{.value=" word "},
          Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d'}}
        },
        .errors={}
      }
    },
    {
      "Немного %1$'#-+10.10d русского %d текста вам в тесты",
      PR{
        .parts={
          Part{.value="Немного "},
          Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d', .width=10, .precision=10, .arg_num=1, .with_precision=true, .show_sign=true, .pad_right=true, .placeholder='#'}},
          Part{.value=" русского "},
          Part{.specifier=Spec{.type=SpecType::Decimal, .fact_symbol='d'}},
          Part{.value=" текста вам в тесты"}
        },
        .errors={}
      }
    },
  };

  for (int test_index = 0; test_index < cases.size(); ++test_index) {
    const auto &test_case = cases[test_index];

    const auto have = FormatString::parse_format(test_case.format);
    ASSERT_EQ_FORMAT(have.parts.size(), test_case.result.parts.size(), test_index, test_case.format);

    for (int i = 0; i < std::min(have.parts.size(), test_case.result.parts.size()); ++i) {
      const auto expected_part = test_case.result.parts[i];
      const auto have_part = have.parts[i];

      ASSERT_EQ_FORMAT(expected_part.value, have_part.value, test_index, test_case.format);

      ASSERT_EQ_FORMAT(expected_part.specifier.type, have_part.specifier.type, test_index, test_case.format);
      ASSERT_EQ_FORMAT(expected_part.specifier.fact_symbol, have_part.specifier.fact_symbol, test_index, test_case.format);
      ASSERT_EQ_FORMAT(expected_part.specifier.width, have_part.specifier.width, test_index, test_case.format);
      ASSERT_EQ_FORMAT(expected_part.specifier.precision, have_part.specifier.precision, test_index, test_case.format);
      ASSERT_EQ_FORMAT(expected_part.specifier.arg_num, have_part.specifier.arg_num, test_index, test_case.format);
      ASSERT_EQ_FORMAT(expected_part.specifier.with_precision, have_part.specifier.with_precision, test_index, test_case.format);
      ASSERT_EQ_FORMAT(expected_part.specifier.show_sign, have_part.specifier.show_sign, test_index, test_case.format);
      ASSERT_EQ_FORMAT(expected_part.specifier.pad_right, have_part.specifier.pad_right, test_index, test_case.format);
      ASSERT_EQ_FORMAT(expected_part.specifier.placeholder, have_part.specifier.placeholder, test_index, test_case.format);

      ASSERT_STREQ_FORMAT(expected_part.specifier.to_string().c_str(), have_part.specifier.to_string().c_str(), test_index, test_case.format);
    }

    ASSERT_EQ_FORMAT(have.errors.size(), test_case.result.errors.size(), test_index, test_case.format);

    for (int i = 0; i < std::min(have.errors.size(), test_case.result.errors.size()); ++i) {
      ASSERT_STREQ_FORMAT(have.errors[i].c_str(), test_case.result.errors[i].c_str(), test_index, test_case.format);
    }
  }
}
