// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <gtest/gtest.h>

#include "compiler/data/ffi-data.h"
#include "compiler/ffi/c_parser/lexer.h"
#include "compiler/ffi/ffi_parser.h"

static std::string ffi_test_join_types(const FFIParseResult &parse_result) {
  std::vector<const FFIType *> types;
  for (const FFIType *type : parse_result) {
    types.emplace_back(type);
  }
  std::string result;
  for (int i = 0; i < types.size(); i++) {
    result += ffi_type_string(types[i]);
    if (i != types.size() - 1) {
      result += "; ";
    }
  }
  return result;
}

TEST(ffi_test, test_lexer) {
  using token_type = ffi::YYParser::token_type;

  struct testCase {
    std::string input;
    std::vector<int> expect;
  };
  std::vector<testCase> tests = {
    {"register", {token_type::C_TOKEN_REGISTER}},
    {"auto", {token_type::C_TOKEN_AUTO}},
    {"extern", {token_type::C_TOKEN_EXTERN}},
    {"enum", {token_type::C_TOKEN_ENUM}},
    {"static", {token_type::C_TOKEN_STATIC}},
    {"inline", {token_type::C_TOKEN_INLINE}},
    {"typedef", {token_type::C_TOKEN_TYPEDEF}},
    {"const", {token_type::C_TOKEN_CONST}},
    {"volatile", {token_type::C_TOKEN_VOLATILE}},
    {"void", {token_type::C_TOKEN_VOID}},
    {"float", {token_type::C_TOKEN_FLOAT}},
    {"double", {token_type::C_TOKEN_DOUBLE}},
    {"char", {token_type::C_TOKEN_CHAR}},
    {"bool", {token_type::C_TOKEN_BOOL}},
    {"int", {token_type::C_TOKEN_INT}},
    {"int8_t", {token_type::C_TOKEN_INT8}},
    {"int16_t", {token_type::C_TOKEN_INT16}},
    {"int32_t", {token_type::C_TOKEN_INT32}},
    {"int64_t", {token_type::C_TOKEN_INT64}},
    {"uint8_t", {token_type::C_TOKEN_UINT8}},
    {"uint16_t", {token_type::C_TOKEN_UINT16}},
    {"uint32_t", {token_type::C_TOKEN_UINT32}},
    {"uint64_t", {token_type::C_TOKEN_UINT64}},
    {"intptr_t", {token_type::C_TOKEN_INTPTR_T}},
    {"uintptr_t", {token_type::C_TOKEN_UINTPTR_T}},
    {"size_t", {token_type::C_TOKEN_SIZE_T}},
    {"long", {token_type::C_TOKEN_LONG}},
    {"short", {token_type::C_TOKEN_SHORT}},
    {"unsigned", {token_type::C_TOKEN_UNSIGNED}},
    {"union", {token_type::C_TOKEN_UNION}},
    {"signed", {token_type::C_TOKEN_SIGNED}},
    {"struct", {token_type::C_TOKEN_STRUCT}},

    {";", {token_type::C_TOKEN_SEMICOLON}},
    {"{", {token_type::C_TOKEN_LBRACE}},
    {"}", {token_type::C_TOKEN_RBRACE}},
    {"(", {token_type::C_TOKEN_LPAREN}},
    {")", {token_type::C_TOKEN_RPAREN}},
    {"[", {token_type::C_TOKEN_LBRACKET}},
    {"]", {token_type::C_TOKEN_RBRACKET}},
    {",", {token_type::C_TOKEN_COMMA}},
    {"*", {token_type::C_TOKEN_MUL}},
    {"=", {token_type::C_TOKEN_EQUAL}},
    {"-", {token_type::C_TOKEN_MINUS}},
    {"+", {token_type::C_TOKEN_PLUS}},

    {"...", {token_type::C_TOKEN_ELLIPSIS}},

    {"foo bar", {token_type::C_TOKEN_IDENTIFIER, token_type::C_TOKEN_IDENTIFIER}},

    {"foo()", {token_type::C_TOKEN_IDENTIFIER, token_type::C_TOKEN_LPAREN, token_type::C_TOKEN_RPAREN}},
    {"foo[0]", {token_type::C_TOKEN_IDENTIFIER, token_type::C_TOKEN_LBRACKET, token_type::C_TOKEN_INT_CONSTANT, token_type::C_TOKEN_RBRACKET}},

    {"2384", {token_type::C_TOKEN_INT_CONSTANT}},
    {"35.72", {token_type::C_TOKEN_FLOAT_CONSTANT}},

    {"-10", {token_type::C_TOKEN_MINUS, token_type::C_TOKEN_INT_CONSTANT}},
    {"+10", {token_type::C_TOKEN_PLUS, token_type::C_TOKEN_INT_CONSTANT}},

    {"10 ;", {token_type::C_TOKEN_INT_CONSTANT, token_type::C_TOKEN_SEMICOLON}},
  };

  const auto tokenize = [](const std::string &input) {
    FFITypedefs typedefs;
    std::vector<int> result;
    ffi::Lexer lexer(typedefs, input);
    while (true) {
      ffi::YYParser::semantic_type sym;
      ffi::YYParser::location_type loc;
      auto tok = lexer.get_next_token(&sym, &loc);
      if (tok == token_type::C_TOKEN_END) {
        break;
      }
      result.push_back(tok);
    }
    return result;
  };

  for (const auto &test : tests) {
    std::vector<std::string> input_variants = {
      test.input,
      " " + test.input,
      test.input + " ",
      test.input + " \0 junk",
    };
    for (const auto &input : input_variants) {
      const auto result = tokenize(input);
      ASSERT_EQ(result, test.expect) << "input string is `" + input + "`";
    }
  }
}

TEST(ffi_test, test_preprocess) {
  struct testCase {
    std::string input;
    std::string output;
  };
  std::vector<testCase> tests = {
    {
      R"(#define FFI_SCOPE "example"
void f();)",
      R"(
void f();)",
    },

    {
      R"(#define FFI_SCOPE "example"
#define FFI_LIB "./ffilibs/libexample"

void f();)",
      R"(


void f();)",
    },
  };

  for (const auto &test : tests) {
    FFIParseResult parse_result;
    std::string preprocessed = ffi_preprocess_file(test.input, parse_result);

    ASSERT_EQ(preprocessed, test.output) << "input is `" + test.input + "`";
  }
}

TEST(ffi_test, test_parse_decl_error) {
  struct testCase {
    std::string input;
    std::string error;
    int64_t error_line = 1;
  };
  std::vector<testCase> tests = {
    // empty declarations with storage classes are permitted in C,
    // but they're useless and we don't allow them
    {"typedef;", "syntax error, unexpected ;"},
    {"extern;", "syntax error, unexpected ;"},
    {"static;", "syntax error, unexpected ;"},

    // we don't permit a storage class specifier to be in a random position,
    // it should be the first keyword
    {"int static x;", "syntax error, unexpected STATIC, expecting ; or ( or * or IDENTIFIER"},
    {"int extern x;", "syntax error, unexpected EXTERN, expecting ; or ( or * or IDENTIFIER"},
    {"int auto x;", "syntax error, unexpected AUTO, expecting ; or ( or * or IDENTIFIER"},
    {"int register x;", "syntax error, unexpected REGISTER, expecting ; or ( or * or IDENTIFIER"},

    {"void f(", "syntax error, unexpected end of file"},
    {"void f()", "syntax error, unexpected end of file, expecting \",\" or ;"},
    {"void f(,)", "syntax error, unexpected \",\""},
    {"void long();", "syntax error, unexpected ), expecting ( or * or IDENTIFIER"},

    // unnamed function pointer params are not accepted by C compilers
    {"void f(void (*) ());", "syntax error, unexpected ), expecting ( or IDENTIFIER"},

    {"void f(int32_t (*g[3]) (), int32_t x)", "parsing function pointer: only simple identifiers are supported (use typedef for arrays)"},

    // variadic '...' can't be used without at least 1 named param
    {"void f(...);", "syntax error, unexpected ELLIPSIS"},

    {
      R"(struct Foo {
        int64_t int
      };)",
      "syntax error, unexpected }, expecting ( or * or IDENTIFIER",
      3,
    },
    {
      R"(struct Foo {
        int64_t int;
      };)",
      "syntax error, unexpected ;, expecting ( or * or IDENTIFIER",
      2,
    },
    {
      R"(
        iii f();
      )",
      "syntax error, unexpected IDENTIFIER",
      2,
    },
    {
      R"(

iii f();
)",
      "syntax error, unexpected IDENTIFIER",
      3,
    },
    {
      R"(#define FFI_LIB "./ffilibs/libexample"
#define FFI_SCOPE "example"

iii f();
)",
      "syntax error, unexpected IDENTIFIER",
      4,
    },
  };

  for (const auto &test : tests) {
    FFITypedefs typedefs;
    auto result = ffi_parse_file(test.input, typedefs);

    ASSERT_EQ(result.err.line, test.error_line) << "input is `" + test.input + "`";
    ASSERT_EQ(result.err.message, test.error) << "input is `" + test.input + "`";
  }
}

TEST(ffi_test, test_parse_enum) {
  struct testCase {
    std::string input;
    std::vector<std::pair<std::string, int>> expected_constants;
  };
  std::vector<testCase> tests = {
    {
      "enum { A, B };",
      {{"A", 0}, {"B", 1}},
    },

    {
      "enum { A = 1, B, C };",
      {{"A", 1}, {"B", 2}, {"C", 3}},
    },

    {
      "enum { A, B = 3, C, D = 6, E, };",
      {{"A", 0}, {"B", 3}, {"C", 4}, {"D", 6}, {"E", 7}},
    },

    {
      "enum { A, B = 3, C = 4, D, E, };",
      {{"A", 0}, {"B", 3}, {"C", 4}, {"D", 5}, {"E", 6}},
    },
  };

  for (const auto &test : tests) {
    FFITypedefs typedefs;
    auto result = ffi_parse_file(test.input, typedefs);
    ASSERT_EQ(result.err.message, "");

    std::vector<std::pair<std::string, int>> result_constants;
    copy(result.enum_constants.begin(), result.enum_constants.end(), std::back_inserter(result_constants));

    ASSERT_EQ(result_constants, test.expected_constants) << "input is " + test.input;
  }
}

TEST(ffi_test, test_parse_sized_int_type_expr) {
  size_t zero = 0; // used to confuse the compiler so we don't get "condition is always false/true" below
  const size_t int_size = sizeof(int) + zero;
  const size_t long_size = sizeof(long) + zero;
  if (int_size != 4 || long_size != 8) {
    // TODO: write tests for 32-bit platforms?
    // TODO: remove ifdef when we update GoogleTest lib on CI
#ifdef GTEST_SKIP
    GTEST_SKIP() << "integer type sizes don't match the test expectations";
#else
    return;
#endif
  }

  struct testCase {
    std::string input;
    std::string expected;
  };
  std::vector<testCase> tests = {
    {"int", "int32_t"},

    {"short", "int16_t"},
    {"long", "int64_t"},

    {"unsigned char", "uint8_t"},
    {"short unsigned", "uint16_t"},
    {"unsigned short", "uint16_t"},
    {"unsigned", "uint32_t"},
    {"long unsigned", "uint64_t"},
    {"unsigned long", "uint64_t"},
    {"long long unsigned", "uint64_t"},
    {"long unsigned long", "uint64_t"},
    {"long unsigned long int", "uint64_t"},
    {"long unsigned int long", "uint64_t"},

    {"signed char", "int8_t"},
    {"short signed", "int16_t"},
    {"signed short", "int16_t"},
    {"signed", "int32_t"},
    {"long signed", "int64_t"},
    {"signed long", "int64_t"},
    {"long long signed", "int64_t"},
    {"long signed long", "int64_t"},

    // enums are simplified to "int"
    {"enum Foo", "int32_t"},

    {"const", "const int32_t"},
    {"const unsigned long long * const * const x", "const uint64_t**"},

    {"size_t", "uint64_t"},
    {"intptr_t", "int64_t"},
    {"uintptr_t", "uint64_t"},
  };

  for (const auto &test : tests) {
    FFITypedefs typedefs;
    auto [target_type, err] = ffi_parse_type(test.input, typedefs);
    ASSERT_EQ(err.message, "");
    ASSERT_EQ(ffi_type_string(target_type), test.expected) << "input expr is " + test.input;
  }
}

TEST(ffi_test, test_parse_simple_type_expr) {
  struct testCase {
    std::string input;
    std::string expected;
  };
  std::vector<testCase> tests = {
    {"void", "void"},
    {"char", "char"},
    {"bool", "bool"},
    {"float", "float"},
    {"double", "double"},

    {"int8_t", "int8_t"},
    {"int16_t", "int16_t"},
    {"int32_t", "int32_t"},
    {"int64_t", "int64_t"},
    {"uint8_t", "uint8_t"},
    {"uint16_t", "uint16_t"},
    {"uint32_t", "uint32_t"},
    {"uint64_t", "uint64_t"},

    {"const int8_t", "const int8_t"},
    {"volatile int8_t", "volatile int8_t"},
    {"int8_t const", "const int8_t"},
    {"int8_t volatile", "volatile int8_t"},
    {"const int8_t const", "const int8_t"},
    {"volatile int8_t volatile", "volatile int8_t"},

    {"int32_t*", "int32_t*"},
    {"const int32_t*", "const int32_t*"},
    {"const int32_t* const", "const int32_t*"},
    {"const int32_t* const **", "const int32_t***"},

    {"struct foo", "struct foo"},
    {"struct foo*", "struct foo*"},

    {"int32_t[]", "int32_t[]"},
    {"uint8_t*[]", "uint8_t*[]"},
    {"uint8_t**[]", "uint8_t**[]"},
    {"const uint8_t*[]", "const uint8_t*[]"},
    {"const uint8_t**[]", "const uint8_t**[]"},
    {"uint8_t[111][]", "uint8_t[111][]"},
    {"uint8_t[111][2][]", "uint8_t[111][2][]"},
    {"uint8_t[111][2][33][]", "uint8_t[111][2][33][]"},
    {"uint8_t*[111][2][33][]", "uint8_t*[111][2][33][]"},
    {"const uint8_t*[111][2][33][]", "const uint8_t*[111][2][33][]"},
  };

  for (const auto &test : tests) {
    FFITypedefs typedefs;
    auto [target_type, err] = ffi_parse_type(test.input, typedefs);
    ASSERT_EQ(err.message, "") << "input expr is " + test.input;
    ASSERT_EQ(ffi_type_string(target_type), test.expected) << "input expr is " + test.input;
  }
}

TEST(ffi_test, test_parse_decl) {
  struct testCase {
    std::string input;
    std::string expected;
  };
  std::vector<testCase> tests = {
    {"char x;", "char x"},

    // using storage class specifiers (that we don't care about)
    {"extern char x;", "char x"},
    {"auto char x;", "char x"},
    {"static char x;", "char x"},
    {"register char x;", "char x"},

    {"const char x, y;", "const char x; const char y"},

    {"char *x;", "char* x"},
    {"char **x;", "char** x"},
    {"char **x, *y, z;", "char** x; char* y; char z"},
    {"char x, *y, **z;", "char x; char* y; char** z"},

    {"char *const x;", "char* x"},
    {"char *const volatile x;", "char* x"},
    {"char *volatile const  x;", "char* x"},

    {"const char *x, *y;", "const char* x; const char* y"},
    {"const char * const x, * volatile y;", "const char* x; const char* y"},
    {"const char ** const x, * const volatile const y;", "const char** x; const char* y"},
    {"const char * const * const * const x;", "const char*** x"},
    {"const char * const * const * const x, y;", "const char*** x; const char y"},
    {"const char * const * const * const x, *y;", "const char*** x; const char* y"},

    // TODO: this should be rejected; only leftmost array type can be unsized
    {"int x[][];", "int32_t x[][]"},

    {"int x[10];", "int32_t x[10]"},
    {"int x[10][20];", "int32_t x[10][20]"},
    {"int x[10][20][3];", "int32_t x[10][20][3]"},
    {"int x[10][20][3][];", "int32_t x[10][20][3][]"},

    {"struct Foo *fooptr;", "struct Foo* fooptr"},
    {"struct Foo * const *fooptr;", "struct Foo** fooptr"},
    {"const struct Foo * const *fooptr;", "const struct Foo** fooptr"},

    {"struct Foo foo_array[10];", "struct Foo foo_array[10]"},
    {"struct Foo foo_array[10][20];", "struct Foo foo_array[10][20]"},

    {"struct { int16_t *arr[11]; };", "struct { int16_t* arr[11]; }"},
    {"struct { const int16_t *arr[11]; };", "struct { const int16_t* arr[11]; }"},
    {"struct { int16_t * arr[11][2]; };", "struct { int16_t* arr[11][2]; }"},
    {"struct { const int16_t *arr[11][2]; };", "struct { const int16_t* arr[11][2]; }"},
    {"struct { const int16_t *arr[11][2][33]; };", "struct { const int16_t* arr[11][2][33]; }"},

    {"struct { int16_t *arr[11][]; };", "struct { int16_t* arr[11][]; }"},
    {"struct { const int16_t *arr[11][]; };", "struct { const int16_t* arr[11][]; }"},
    {"struct { int16_t* arr [11] [222] []; };", "struct { int16_t* arr[11][222][]; }"},
    {"struct { struct Foo arr[11] [222] []; };", "struct { struct Foo arr[11][222][]; }"},
    {"struct { const struct Foo *arr[11] [222] []; };", "struct { const struct Foo* arr[11][222][]; }"},
    {"int x[];", "int32_t x[]"},

    {"struct { int8_t x; };", "struct { int8_t x; }"},
    {"struct { int8_t x, y; };", "struct { int8_t x; int8_t y; }"},
    {"struct { int8_t x; int16_t y; };", "struct { int8_t x; int16_t y; }"},

    {"struct { int8_t *x, y; };", "struct { int8_t* x; int8_t y; }"},
    {"struct { int8_t x, *y; };", "struct { int8_t x; int8_t* y; }"},
    {"struct { int8_t *x, **y; };", "struct { int8_t* x; int8_t** y; }"},

    {"struct { const int8_t *x, y; };", "struct { const int8_t* x; const int8_t y; }"},
    {"struct { volatile const int8_t *x, y; };", "struct { const volatile int8_t* x; const volatile int8_t y; }"},
    {"struct { int8_t x, * const y; };", "struct { int8_t x; int8_t* y; }"},
    {"struct { int8_t * const x, * const y; };", "struct { int8_t* x; int8_t* y; }"},

    {"struct { int8_t x[10], y; };", "struct { int8_t x[10]; int8_t y; }"},
    {"struct { int8_t x, y[1][2]; };", "struct { int8_t x; int8_t y[1][2]; }"},
    {"struct { int8_t x[1][2], y[3]; };", "struct { int8_t x[1][2]; int8_t y[3]; }"},

    {"struct foo { char x; };", "struct foo{ char x; }"},
    {"struct foo { char x; int8_t y; };", "struct foo{ char x; int8_t y; }"},

    {"struct foo { void (*f) (int32_t x); };", "struct foo{ void (*f)(int32_t x); }"},
    {"struct foo { void (*f) (int64_t, double); };", "struct foo{ void (*f)(int64_t, double); }"},
    {"struct foo { struct bar* (*g) (); };", "struct foo{ struct bar* (*g)(); }"},

    {"void f(int32_t code, ...);", "void f(int32_t code, ...)"},
    {"void f(int32_t fd, const char *format, ...);", "void f(int32_t fd, const char* format, ...)"},

    {"void f(void);", "void f()"},
    {"int32_t f(void);", "int32_t f()"},

    {"void f(void (*g) ());", "void f(void (*g)())"},
    {"void f(void (*g) (int32_t));", "void f(void (*g)(int32_t))"},
    {"void f(void (*g) (int32_t x));", "void f(void (*g)(int32_t x))"},
    {"void f(void (*g) (int32_t, int64_t));", "void f(void (*g)(int32_t, int64_t))"},
    {"void f(void (*g) (int32_t x, int64_t y));", "void f(void (*g)(int32_t x, int64_t y))"},
    {"float f(double (*g) (int32_t x, int64_t y));", "float f(double (*g)(int32_t x, int64_t y))"},
    {"void f(void (*g1) (), int32_t (*g2) (int64_t), int32_t x);", "void f(void (*g1)(), int32_t (*g2)(int64_t), int32_t x)"},
  };

  for (const auto &test : tests) {
    FFITypedefs typedefs;
    std::string input = test.input;
    auto result = ffi_parse_file(input, typedefs);

    ASSERT_EQ(result.err.message, "");
    std::string declarations = ffi_test_join_types(result);
    ASSERT_EQ(declarations, test.expected) << "input decl is " + test.input;
  }
}

TEST(ffi_test, test_decltype_string) {
  struct testCase {
    std::string input;
    std::string expected;
  };
  std::vector<testCase> tests = {
    {"int x[10]", "int32_t[10]"},
    {"int x[10][20]", "int32_t[10][20]"},
    {"int x[10][20][3]", "int32_t[10][20][3]"},

    {"int32_t x[]", "int32_t[]"},
    {"int32_t x[1][]", "int32_t[1][]"},
    {"int32_t x[1][2][]", "int32_t[1][2][]"},

    {"struct Foo foo_array[10]", "struct Foo[10]"},
    {"struct Foo foo_array[10][20]", "struct Foo[10][20]"},
  };

  for (const auto &test : tests) {
    FFITypedefs typedefs;
    auto [target_type, err] = ffi_parse_type(test.input, typedefs);
    ASSERT_EQ(err.message, "");
    ASSERT_EQ(ffi_decltype_string(target_type), test.expected) << "input expr is " + test.input;
  }
}

TEST(ffi_test, test_typedef) {
  struct testCase {
    std::string input;
    std::string expected;
  };
  std::vector<testCase> tests = {
    // simple typedefs and usages
    {"typedef int16_t i16; void f(i16 x);", "void f(int16_t x)"},

    // typedef on top of another typedef
    {
      "typedef int32_t int32; typedef int32 i32; void f(i32 x);",
      "void f(int32_t x)",
    },
    {
      "typedef int32_t int32; typedef int32 i32; void f(int32 x);",
      "void f(int32_t x)",
    },

    // typedef on a struct type
    {
      R"(
        typedef struct Vector Vector;
        struct Vector { float x; };
        void f(Vector x);
      )",
      "struct Vector{ float x; }; void f(struct Vector x)",
    },
    {
      R"(
        struct Vector { float x; };
        typedef struct Vector Vector;
        void f(Vector x);
      )",
      "struct Vector{ float x; }; void f(struct Vector x)",
    },

    // typedef on inline struct type
    {
      "typedef struct Foo { int64_t x; } Foo; void f(Foo x);",
      "struct Foo{ int64_t x; }; void f(struct Foo x)",
    },
    {
      "typedef struct { int64_t x; } Foo; void f(Foo x);",
      "struct Foo{ int64_t x; }; void f(struct Foo x)",
    },

    // typedef + qualifiers
    {
      "typedef int64_t i64; void f(const i64* arg);",
      "void f(const int64_t* arg)",
    },
    {
      "typedef int64_t i64; void f(volatile i64* arg);",
      "void f(volatile int64_t* arg)",
    },

    // typedef on array types
    {
      "typedef uint8_t my_int128[16]; void f(my_int128 arg);",
      "void f(uint8_t arg[16])",
    },
    {
      "typedef uint8_t foo[2][4]; void f(foo arg);",
      "void f(uint8_t arg[2][4])",
    },
    {
      "typedef uint8_t mem[]; void f(mem arg);",
      "void f(uint8_t arg[])",
    },

    // typedef on function pointer types
    {
      "typedef void (*FuncType) (int32_t); void f(FuncType g);",
      "void f(void (*g)(int32_t))",
    },
    {
      "typedef double (*FuncType) (int8_t x, int16_t y); void f(FuncType g);",
      "void f(double (*g)(int8_t x, int16_t y))",
    },

    // arrays of pointers
    {
      "typedef uint8_t *foo[8]; void f(foo arg);",
      "void f(uint8_t* arg[8])",
    },
    {
      "typedef uint8_t *mem[]; void f(mem arg);",
      "void f(uint8_t* arg[])",
    },

    // combining things together
    {
      R"(
        typedef struct Vector Vector;
        typedef Vector *VectorPtr;
        struct Vector { float x; };
        void f(VectorPtr x);
      )",
      "struct Vector{ float x; }; void f(struct Vector* x)",
    },
    {
      R"(
        struct Vector { float x; };
        typedef struct Vector Vector;
        typedef Vector *VectorPtr;
        typedef VectorPtr *VectorPtrPtr;
        void f(VectorPtrPtr x);
      )",
      "struct Vector{ float x; }; void f(struct Vector** x)",
    },
    {
      R"(
        typedef int64_t i64;
        typedef struct { i64 x, y; } Foo;
        Foo* f();
      )",
      "struct Foo{ int64_t x; int64_t y; }; struct Foo* f()",
    },
    {
      R"(
        typedef int64_t i64;
        typedef i64 (*FuncType) (i64);
        typedef void (*FuncTypeTwo) (FuncType);
        void f(FuncTypeTwo g);
      )",
      "void f(void (*g)(int64_t (*)(int64_t)))",
    },
    {
      R"(
        typedef int64_t i64;
        typedef i64 (*FuncType) (i64 x);
        typedef void (*FuncTypeTwo) (FuncType callback);
        void f(FuncTypeTwo funcs[2]);
      )",
      "void f(void (*funcs[2])(int64_t (*callback)(int64_t x)))",
    },
  };

  for (const auto &test : tests) {
    FFITypedefs typedefs;
    std::string input = test.input;
    auto result = ffi_parse_file(input, typedefs);

    ASSERT_EQ(result.err.message, "");
    std::string declarations = ffi_test_join_types(result);
    ASSERT_EQ(declarations, test.expected) << "input decl is " + test.input;
  }
}
