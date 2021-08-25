// A Bison parser, made by GNU Bison 3.7.

// Skeleton interface for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2020 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.


/**
 ** \file yy_parser_generated.hpp
 ** Define the ffi::parser class.
 */

// C++ LALR(1) parser skeleton written by Akim Demaille.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.

#ifndef YY_YY_YY_PARSER_GENERATED_HPP_INCLUDED
# define YY_YY_YY_PARSER_GENERATED_HPP_INCLUDED
// "%code requires" blocks.
#line 23 "c.y"

  #include <iostream>
  #include <string>
  #include <vector>
  #include <stdint.h>

  #include "compiler/ffi/ffi_types.h"
  #include "compiler/ffi/c_parser/location.h"
  #include "compiler/ffi/c_parser/parsing_types.h"
  #include "common/wrappers/string_view.h"

  namespace ffi {
    class Lexer;
    class ParsingDriver;
  }

#line 66 "yy_parser_generated.hpp"


# include <cstdlib> // std::abort
# include <iostream>
# include <stdexcept>
# include <string>
# include <vector>

#if defined __cplusplus
# define YY_CPLUSPLUS __cplusplus
#else
# define YY_CPLUSPLUS 199711L
#endif

// Support move semantics when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_MOVE           std::move
# define YY_MOVE_OR_COPY   move
# define YY_MOVE_REF(Type) Type&&
# define YY_RVREF(Type)    Type&&
# define YY_COPY(Type)     Type
#else
# define YY_MOVE
# define YY_MOVE_OR_COPY   copy
# define YY_MOVE_REF(Type) Type&
# define YY_RVREF(Type)    const Type&
# define YY_COPY(Type)     const Type&
#endif

// Support noexcept when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_NOEXCEPT noexcept
# define YY_NOTHROW
#else
# define YY_NOEXCEPT
# define YY_NOTHROW throw ()
#endif

// Support constexpr when possible.
#if 201703 <= YY_CPLUSPLUS
# define YY_CONSTEXPR constexpr
#else
# define YY_CONSTEXPR
#endif



#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

#line 13 "c.y"
namespace ffi {
#line 196 "yy_parser_generated.hpp"




  /// A Bison parser.
  class YYParser
  {
  public:
#ifndef YYSTYPE
    /// Symbol semantic values.
    union semantic_type
    {
#line 79 "c.y"

  string_span str_;
  int int_;
  FFIType *type_;

#line 215 "yy_parser_generated.hpp"

    };
#else
    typedef YYSTYPE semantic_type;
#endif
    /// Symbol locations.
    typedef ffi::Location location_type;

    /// Syntax errors thrown from user actions.
    struct syntax_error : std::runtime_error
    {
      syntax_error (const location_type& l, const std::string& m)
        : std::runtime_error (m)
        , location (l)
      {}

      syntax_error (const syntax_error& s)
        : std::runtime_error (s.what ())
        , location (s.location)
      {}

      ~syntax_error () YY_NOEXCEPT YY_NOTHROW;

      location_type location;
    };

    /// Token kinds.
    struct token
    {
      enum token_kind_type
      {
        C_TOKEN_YYEMPTY = -2,
    C_TOKEN_END = 0,               // "end of file"
    C_TOKEN_YYerror = 256,         // error
    C_TOKEN_YYUNDEF = 257,         // "invalid token"
    C_TOKEN_COMMA = 258,           // ","
    C_TOKEN_SEMICOLON = 259,       // ";"
    C_TOKEN_LBRACE = 260,          // "{"
    C_TOKEN_RBRACE = 261,          // "}"
    C_TOKEN_LPAREN = 262,          // "("
    C_TOKEN_RPAREN = 263,          // ")"
    C_TOKEN_LBRACKET = 264,        // "["
    C_TOKEN_RBRACKET = 265,        // "]"
    C_TOKEN_MUL = 266,             // "*"
    C_TOKEN_EQUAL = 267,           // "="
    C_TOKEN_PLUS = 268,            // "+"
    C_TOKEN_MINUS = 269,           // "-"
    C_TOKEN_IDENTIFIER = 270,      // IDENTIFIER
    C_TOKEN_INT_CONSTANT = 271,    // INT_CONSTANT
    C_TOKEN_FLOAT_CONSTANT = 272,  // FLOAT_CONSTANT
    C_TOKEN_STRING_LITERAL = 273,  // STRING_LITERAL
    C_TOKEN_SIZEOF = 274,          // SIZEOF
    C_TOKEN_PTR_OP = 275,          // PTR_OP
    C_TOKEN_INC_OP = 276,          // INC_OP
    C_TOKEN_DEC_OP = 277,          // DEC_OP
    C_TOKEN_LEFT_OP = 278,         // LEFT_OP
    C_TOKEN_RIGHT_OP = 279,        // RIGHT_OP
    C_TOKEN_LE_OP = 280,           // LE_OP
    C_TOKEN_GE_OP = 281,           // GE_OP
    C_TOKEN_EQ_OP = 282,           // EQ_OP
    C_TOKEN_NE_OP = 283,           // NE_OP
    C_TOKEN_AND_OP = 284,          // AND_OP
    C_TOKEN_OR_OP = 285,           // OR_OP
    C_TOKEN_MUL_ASSIGN = 286,      // MUL_ASSIGN
    C_TOKEN_DIV_ASSIGN = 287,      // DIV_ASSIGN
    C_TOKEN_MOD_ASSIGN = 288,      // MOD_ASSIGN
    C_TOKEN_ADD_ASSIGN = 289,      // ADD_ASSIGN
    C_TOKEN_SUB_ASSIGN = 290,      // SUB_ASSIGN
    C_TOKEN_LEFT_ASSIGN = 291,     // LEFT_ASSIGN
    C_TOKEN_RIGHT_ASSIGN = 292,    // RIGHT_ASSIGN
    C_TOKEN_AND_ASSIGN = 293,      // AND_ASSIGN
    C_TOKEN_XOR_ASSIGN = 294,      // XOR_ASSIGN
    C_TOKEN_OR_ASSIGN = 295,       // OR_ASSIGN
    C_TOKEN_TYPEDEF_NAME = 296,    // TYPEDEF_NAME
    C_TOKEN_TYPEDEF = 297,         // TYPEDEF
    C_TOKEN_INLINE = 298,          // INLINE
    C_TOKEN_EXTERN = 299,          // EXTERN
    C_TOKEN_STATIC = 300,          // STATIC
    C_TOKEN_AUTO = 301,            // AUTO
    C_TOKEN_REGISTER = 302,        // REGISTER
    C_TOKEN_THREAD_LOCAL = 303,    // THREAD_LOCAL
    C_TOKEN_BOOL = 304,            // BOOL
    C_TOKEN_CHAR = 305,            // CHAR
    C_TOKEN_SHORT = 306,           // SHORT
    C_TOKEN_INT = 307,             // INT
    C_TOKEN_LONG = 308,            // LONG
    C_TOKEN_SIGNED = 309,          // SIGNED
    C_TOKEN_UNSIGNED = 310,        // UNSIGNED
    C_TOKEN_FLOAT = 311,           // FLOAT
    C_TOKEN_DOUBLE = 312,          // DOUBLE
    C_TOKEN_CONST = 313,           // CONST
    C_TOKEN_VOLATILE = 314,        // VOLATILE
    C_TOKEN_VOID = 315,            // VOID
    C_TOKEN_INT8 = 316,            // INT8
    C_TOKEN_INT16 = 317,           // INT16
    C_TOKEN_INT32 = 318,           // INT32
    C_TOKEN_INT64 = 319,           // INT64
    C_TOKEN_UINT8 = 320,           // UINT8
    C_TOKEN_UINT16 = 321,          // UINT16
    C_TOKEN_UINT32 = 322,          // UINT32
    C_TOKEN_UINT64 = 323,          // UINT64
    C_TOKEN_SIZE_T = 324,          // SIZE_T
    C_TOKEN_STRUCT = 325,          // STRUCT
    C_TOKEN_UNION = 326,           // UNION
    C_TOKEN_ENUM = 327,            // ENUM
    C_TOKEN_ELLIPSIS = 328,        // ELLIPSIS
    C_TOKEN_CASE = 329,            // CASE
    C_TOKEN_DEFAULT = 330,         // DEFAULT
    C_TOKEN_IF = 331,              // IF
    C_TOKEN_ELSE = 332,            // ELSE
    C_TOKEN_SWITCH = 333,          // SWITCH
    C_TOKEN_WHILE = 334,           // WHILE
    C_TOKEN_DO = 335,              // DO
    C_TOKEN_FOR = 336,             // FOR
    C_TOKEN_GOTO = 337,            // GOTO
    C_TOKEN_CONTINUE = 338,        // CONTINUE
    C_TOKEN_BREAK = 339,           // BREAK
    C_TOKEN_RETURN = 340           // RETURN
      };
      /// Backward compatibility alias (Bison 3.6).
      typedef token_kind_type yytokentype;
    };

    /// Token kind, as returned by yylex.
    typedef token::yytokentype token_kind_type;

    /// Backward compatibility alias (Bison 3.6).
    typedef token_kind_type token_type;

    /// Symbol kinds.
    struct symbol_kind
    {
      enum symbol_kind_type
      {
        YYNTOKENS = 86, ///< Number of tokens.
        S_YYEMPTY = -2,
        S_YYEOF = 0,                             // "end of file"
        S_YYerror = 1,                           // error
        S_YYUNDEF = 2,                           // "invalid token"
        S_COMMA = 3,                             // ","
        S_SEMICOLON = 4,                         // ";"
        S_LBRACE = 5,                            // "{"
        S_RBRACE = 6,                            // "}"
        S_LPAREN = 7,                            // "("
        S_RPAREN = 8,                            // ")"
        S_LBRACKET = 9,                          // "["
        S_RBRACKET = 10,                         // "]"
        S_MUL = 11,                              // "*"
        S_EQUAL = 12,                            // "="
        S_PLUS = 13,                             // "+"
        S_MINUS = 14,                            // "-"
        S_IDENTIFIER = 15,                       // IDENTIFIER
        S_INT_CONSTANT = 16,                     // INT_CONSTANT
        S_FLOAT_CONSTANT = 17,                   // FLOAT_CONSTANT
        S_STRING_LITERAL = 18,                   // STRING_LITERAL
        S_SIZEOF = 19,                           // SIZEOF
        S_PTR_OP = 20,                           // PTR_OP
        S_INC_OP = 21,                           // INC_OP
        S_DEC_OP = 22,                           // DEC_OP
        S_LEFT_OP = 23,                          // LEFT_OP
        S_RIGHT_OP = 24,                         // RIGHT_OP
        S_LE_OP = 25,                            // LE_OP
        S_GE_OP = 26,                            // GE_OP
        S_EQ_OP = 27,                            // EQ_OP
        S_NE_OP = 28,                            // NE_OP
        S_AND_OP = 29,                           // AND_OP
        S_OR_OP = 30,                            // OR_OP
        S_MUL_ASSIGN = 31,                       // MUL_ASSIGN
        S_DIV_ASSIGN = 32,                       // DIV_ASSIGN
        S_MOD_ASSIGN = 33,                       // MOD_ASSIGN
        S_ADD_ASSIGN = 34,                       // ADD_ASSIGN
        S_SUB_ASSIGN = 35,                       // SUB_ASSIGN
        S_LEFT_ASSIGN = 36,                      // LEFT_ASSIGN
        S_RIGHT_ASSIGN = 37,                     // RIGHT_ASSIGN
        S_AND_ASSIGN = 38,                       // AND_ASSIGN
        S_XOR_ASSIGN = 39,                       // XOR_ASSIGN
        S_OR_ASSIGN = 40,                        // OR_ASSIGN
        S_TYPEDEF_NAME = 41,                     // TYPEDEF_NAME
        S_TYPEDEF = 42,                          // TYPEDEF
        S_INLINE = 43,                           // INLINE
        S_EXTERN = 44,                           // EXTERN
        S_STATIC = 45,                           // STATIC
        S_AUTO = 46,                             // AUTO
        S_REGISTER = 47,                         // REGISTER
        S_THREAD_LOCAL = 48,                     // THREAD_LOCAL
        S_BOOL = 49,                             // BOOL
        S_CHAR = 50,                             // CHAR
        S_SHORT = 51,                            // SHORT
        S_INT = 52,                              // INT
        S_LONG = 53,                             // LONG
        S_SIGNED = 54,                           // SIGNED
        S_UNSIGNED = 55,                         // UNSIGNED
        S_FLOAT = 56,                            // FLOAT
        S_DOUBLE = 57,                           // DOUBLE
        S_CONST = 58,                            // CONST
        S_VOLATILE = 59,                         // VOLATILE
        S_VOID = 60,                             // VOID
        S_INT8 = 61,                             // INT8
        S_INT16 = 62,                            // INT16
        S_INT32 = 63,                            // INT32
        S_INT64 = 64,                            // INT64
        S_UINT8 = 65,                            // UINT8
        S_UINT16 = 66,                           // UINT16
        S_UINT32 = 67,                           // UINT32
        S_UINT64 = 68,                           // UINT64
        S_SIZE_T = 69,                           // SIZE_T
        S_STRUCT = 70,                           // STRUCT
        S_UNION = 71,                            // UNION
        S_ENUM = 72,                             // ENUM
        S_ELLIPSIS = 73,                         // ELLIPSIS
        S_CASE = 74,                             // CASE
        S_DEFAULT = 75,                          // DEFAULT
        S_IF = 76,                               // IF
        S_ELSE = 77,                             // ELSE
        S_SWITCH = 78,                           // SWITCH
        S_WHILE = 79,                            // WHILE
        S_DO = 80,                               // DO
        S_FOR = 81,                              // FOR
        S_GOTO = 82,                             // GOTO
        S_CONTINUE = 83,                         // CONTINUE
        S_BREAK = 84,                            // BREAK
        S_RETURN = 85,                           // RETURN
        S_YYACCEPT = 86,                         // $accept
        S_translation_unit = 87,                 // translation_unit
        S_external_declaration = 88,             // external_declaration
        S_typedef_declaration = 89,              // typedef_declaration
        S_declaration = 90,                      // declaration
        S_declaration_specifiers = 91,           // declaration_specifiers
        S_type_qualifier = 92,                   // type_qualifier
        S_base_type_specifier = 93,              // base_type_specifier
        S_type_specifier = 94,                   // type_specifier
        S_init_declarator_list = 95,             // init_declarator_list
        S_init_declarator = 96,                  // init_declarator
        S_declarator = 97,                       // declarator
        S_pointer = 98,                          // pointer
        S_type_qualifier_list = 99,              // type_qualifier_list
        S_direct_declarator = 100,               // direct_declarator
        S_parameter_type_list = 101,             // parameter_type_list
        S_parameter_list = 102,                  // parameter_list
        S_parameter_declaration = 103,           // parameter_declaration
        S_abstract_declarator = 104,             // abstract_declarator
        S_struct_or_union_specifier = 105,       // struct_or_union_specifier
        S_typedef_name_or_identifier = 106,      // typedef_name_or_identifier
        S_struct_declaration_list = 107,         // struct_declaration_list
        S_struct_declaration = 108,              // struct_declaration
        S_specifier_qualifier_list = 109,        // specifier_qualifier_list
        S_struct_declarator_list = 110,          // struct_declarator_list
        S_struct_declarator = 111,               // struct_declarator
        S_enum_specifier = 112,                  // enum_specifier
        S_enumerator_list = 113,                 // enumerator_list
        S_enumerator = 114,                      // enumerator
        S_int_value = 115                        // int_value
      };
    };

    /// (Internal) symbol kind.
    typedef symbol_kind::symbol_kind_type symbol_kind_type;

    /// The number of tokens.
    static const symbol_kind_type YYNTOKENS = symbol_kind::YYNTOKENS;

    /// A complete symbol.
    ///
    /// Expects its Base type to provide access to the symbol kind
    /// via kind ().
    ///
    /// Provide access to semantic value and location.
    template <typename Base>
    struct basic_symbol : Base
    {
      /// Alias to Base.
      typedef Base super_type;

      /// Default constructor.
      basic_symbol ()
        : value ()
        , location ()
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      basic_symbol (basic_symbol&& that)
        : Base (std::move (that))
        , value (std::move (that.value))
        , location (std::move (that.location))
      {}
#endif

      /// Copy constructor.
      basic_symbol (const basic_symbol& that);
      /// Constructor for valueless symbols.
      basic_symbol (typename Base::kind_type t,
                    YY_MOVE_REF (location_type) l);

      /// Constructor for symbols with semantic value.
      basic_symbol (typename Base::kind_type t,
                    YY_RVREF (semantic_type) v,
                    YY_RVREF (location_type) l);

      /// Destroy the symbol.
      ~basic_symbol ()
      {
        clear ();
      }

      /// Destroy contents, and record that is empty.
      void clear ()
      {
        Base::clear ();
      }

      /// The user-facing name of this symbol.
      std::string name () const YY_NOEXCEPT
      {
        return YYParser::symbol_name (this->kind ());
      }

      /// Backward compatibility (Bison 3.6).
      symbol_kind_type type_get () const YY_NOEXCEPT;

      /// Whether empty.
      bool empty () const YY_NOEXCEPT;

      /// Destructive move, \a s is emptied into this.
      void move (basic_symbol& s);

      /// The semantic value.
      semantic_type value;

      /// The location.
      location_type location;

    private:
#if YY_CPLUSPLUS < 201103L
      /// Assignment operator.
      basic_symbol& operator= (const basic_symbol& that);
#endif
    };

    /// Type access provider for token (enum) based symbols.
    struct by_kind
    {
      /// Default constructor.
      by_kind ();

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      by_kind (by_kind&& that);
#endif

      /// Copy constructor.
      by_kind (const by_kind& that);

      /// The symbol kind as needed by the constructor.
      typedef token_kind_type kind_type;

      /// Constructor from (external) token numbers.
      by_kind (kind_type t);

      /// Record that this symbol is empty.
      void clear ();

      /// Steal the symbol kind from \a that.
      void move (by_kind& that);

      /// The (internal) type number (corresponding to \a type).
      /// \a empty when empty.
      symbol_kind_type kind () const YY_NOEXCEPT;

      /// Backward compatibility (Bison 3.6).
      symbol_kind_type type_get () const YY_NOEXCEPT;

      /// The symbol kind.
      /// \a S_YYEMPTY when empty.
      symbol_kind_type kind_;
    };

    /// Backward compatibility for a private implementation detail (Bison 3.6).
    typedef by_kind by_type;

    /// "External" symbols: returned by the scanner.
    struct symbol_type : basic_symbol<by_kind>
    {};

    /// Build a parser object.
    YYParser (ffi::Lexer &lexer_yyarg, ffi::ParsingDriver &driver_yyarg);
    virtual ~YYParser ();

#if 201103L <= YY_CPLUSPLUS
    /// Non copyable.
    YYParser (const YYParser&) = delete;
    /// Non copyable.
    YYParser& operator= (const YYParser&) = delete;
#endif

    /// Parse.  An alias for parse ().
    /// \returns  0 iff parsing succeeded.
    int operator() ();

    /// Parse.
    /// \returns  0 iff parsing succeeded.
    virtual int parse ();

#if YYDEBUG
    /// The current debugging stream.
    std::ostream& debug_stream () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging stream.
    void set_debug_stream (std::ostream &);

    /// Type for debugging levels.
    typedef int debug_level_type;
    /// The current debugging level.
    debug_level_type debug_level () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging level.
    void set_debug_level (debug_level_type l);
#endif

    /// Report a syntax error.
    /// \param loc    where the syntax error is found.
    /// \param msg    a description of the syntax error.
    virtual void error (const location_type& loc, const std::string& msg);

    /// Report a syntax error.
    void error (const syntax_error& err);

    /// The user-facing name of the symbol whose (internal) number is
    /// YYSYMBOL.  No bounds checking.
    static std::string symbol_name (symbol_kind_type yysymbol);



    class context
    {
    public:
      context (const YYParser& yyparser, const symbol_type& yyla);
      const symbol_type& lookahead () const { return yyla_; }
      symbol_kind_type token () const { return yyla_.kind (); }
      const location_type& location () const { return yyla_.location; }

      /// Put in YYARG at most YYARGN of the expected tokens, and return the
      /// number of tokens stored in YYARG.  If YYARG is null, return the
      /// number of expected tokens (guaranteed to be less than YYNTOKENS).
      int expected_tokens (symbol_kind_type yyarg[], int yyargn) const;

    private:
      const YYParser& yyparser_;
      const symbol_type& yyla_;
    };

  private:
#if YY_CPLUSPLUS < 201103L
    /// Non copyable.
    YYParser (const YYParser&);
    /// Non copyable.
    YYParser& operator= (const YYParser&);
#endif


    /// Stored state numbers (used for stacks).
    typedef unsigned char state_type;

    /// The arguments of the error message.
    int yy_syntax_error_arguments_ (const context& yyctx,
                                    symbol_kind_type yyarg[], int yyargn) const;

    /// Generate an error message.
    /// \param yyctx     the context in which the error occurred.
    virtual std::string yysyntax_error_ (const context& yyctx) const;
    /// Compute post-reduction state.
    /// \param yystate   the current state
    /// \param yysym     the nonterminal to push on the stack
    static state_type yy_lr_goto_state_ (state_type yystate, int yysym);

    /// Whether the given \c yypact_ value indicates a defaulted state.
    /// \param yyvalue   the value to check
    static bool yy_pact_value_is_default_ (int yyvalue);

    /// Whether the given \c yytable_ value indicates a syntax error.
    /// \param yyvalue   the value to check
    static bool yy_table_value_is_error_ (int yyvalue);

    static const signed char yypact_ninf_;
    static const signed char yytable_ninf_;

    /// Convert a scanner token kind \a t to a symbol kind.
    /// In theory \a t should be a token_kind_type, but character literals
    /// are valid, yet not members of the token_type enum.
    static symbol_kind_type yytranslate_ (int t);

    /// Convert the symbol name \a n to a form suitable for a diagnostic.
    static std::string yytnamerr_ (const char *yystr);

    /// For a symbol, its name in clear.
    static const char* const yytname_[];


    // Tables.
    // YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
    // STATE-NUM.
    static const short yypact_[];

    // YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
    // Performed when YYTABLE does not specify something else to do.  Zero
    // means the default is an error.
    static const signed char yydefact_[];

    // YYPGOTO[NTERM-NUM].
    static const signed char yypgoto_[];

    // YYDEFGOTO[NTERM-NUM].
    static const short yydefgoto_[];

    // YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
    // positive, shift that token.  If negative, reduce the rule whose
    // number is the opposite.  If YYTABLE_NINF, syntax error.
    static const unsigned char yytable_[];

    static const short yycheck_[];

    // YYSTOS[STATE-NUM] -- The (internal number of the) accessing
    // symbol of state STATE-NUM.
    static const signed char yystos_[];

    // YYR1[YYN] -- Symbol number of symbol that rule YYN derives.
    static const signed char yyr1_[];

    // YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.
    static const signed char yyr2_[];


#if YYDEBUG
    // YYRLINE[YYN] -- Source line where rule number YYN was defined.
    static const short yyrline_[];
    /// Report on the debug stream that the rule \a r is going to be reduced.
    virtual void yy_reduce_print_ (int r) const;
    /// Print the state stack on the debug stream.
    virtual void yy_stack_print_ () const;

    /// Debugging level.
    int yydebug_;
    /// Debug stream.
    std::ostream* yycdebug_;

    /// \brief Display a symbol kind, value and location.
    /// \param yyo    The output stream.
    /// \param yysym  The symbol.
    template <typename Base>
    void yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const;
#endif

    /// \brief Reclaim the memory associated to a symbol.
    /// \param yymsg     Why this token is reclaimed.
    ///                  If null, print nothing.
    /// \param yysym     The symbol.
    template <typename Base>
    void yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const;

  private:
    /// Type access provider for state based symbols.
    struct by_state
    {
      /// Default constructor.
      by_state () YY_NOEXCEPT;

      /// The symbol kind as needed by the constructor.
      typedef state_type kind_type;

      /// Constructor.
      by_state (kind_type s) YY_NOEXCEPT;

      /// Copy constructor.
      by_state (const by_state& that) YY_NOEXCEPT;

      /// Record that this symbol is empty.
      void clear () YY_NOEXCEPT;

      /// Steal the symbol kind from \a that.
      void move (by_state& that);

      /// The symbol kind (corresponding to \a state).
      /// \a symbol_kind::S_YYEMPTY when empty.
      symbol_kind_type kind () const YY_NOEXCEPT;

      /// The state number used to denote an empty symbol.
      /// We use the initial state, as it does not have a value.
      enum { empty_state = 0 };

      /// The state.
      /// \a empty when empty.
      state_type state;
    };

    /// "Internal" symbol: element of the stack.
    struct stack_symbol_type : basic_symbol<by_state>
    {
      /// Superclass.
      typedef basic_symbol<by_state> super_type;
      /// Construct an empty symbol.
      stack_symbol_type ();
      /// Move or copy construction.
      stack_symbol_type (YY_RVREF (stack_symbol_type) that);
      /// Steal the contents from \a sym to build this.
      stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) sym);
#if YY_CPLUSPLUS < 201103L
      /// Assignment, needed by push_back by some old implementations.
      /// Moves the contents of that.
      stack_symbol_type& operator= (stack_symbol_type& that);

      /// Assignment, needed by push_back by other implementations.
      /// Needed by some other old implementations.
      stack_symbol_type& operator= (const stack_symbol_type& that);
#endif
    };

    /// A stack with random access from its top.
    template <typename T, typename S = std::vector<T> >
    class stack
    {
    public:
      // Hide our reversed order.
      typedef typename S::iterator iterator;
      typedef typename S::const_iterator const_iterator;
      typedef typename S::size_type size_type;
      typedef typename std::ptrdiff_t index_type;

      stack (size_type n = 200)
        : seq_ (n)
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Non copyable.
      stack (const stack&) = delete;
      /// Non copyable.
      stack& operator= (const stack&) = delete;
#endif

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      const T&
      operator[] (index_type i) const
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      T&
      operator[] (index_type i)
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Steal the contents of \a t.
      ///
      /// Close to move-semantics.
      void
      push (YY_MOVE_REF (T) t)
      {
        seq_.push_back (T ());
        operator[] (0).move (t);
      }

      /// Pop elements from the stack.
      void
      pop (std::ptrdiff_t n = 1) YY_NOEXCEPT
      {
        for (; 0 < n; --n)
          seq_.pop_back ();
      }

      /// Pop all elements from the stack.
      void
      clear () YY_NOEXCEPT
      {
        seq_.clear ();
      }

      /// Number of elements on the stack.
      index_type
      size () const YY_NOEXCEPT
      {
        return index_type (seq_.size ());
      }

      /// Iterator on top of the stack (going downwards).
      const_iterator
      begin () const YY_NOEXCEPT
      {
        return seq_.begin ();
      }

      /// Bottom of the stack.
      const_iterator
      end () const YY_NOEXCEPT
      {
        return seq_.end ();
      }

      /// Present a slice of the top of a stack.
      class slice
      {
      public:
        slice (const stack& stack, index_type range)
          : stack_ (stack)
          , range_ (range)
        {}

        const T&
        operator[] (index_type i) const
        {
          return stack_[range_ - i];
        }

      private:
        const stack& stack_;
        index_type range_;
      };

    private:
#if YY_CPLUSPLUS < 201103L
      /// Non copyable.
      stack (const stack&);
      /// Non copyable.
      stack& operator= (const stack&);
#endif
      /// The wrapped container.
      S seq_;
    };


    /// Stack type.
    typedef stack<stack_symbol_type> stack_type;

    /// The stack.
    stack_type yystack_;

    /// Push a new state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param sym  the symbol
    /// \warning the contents of \a s.value is stolen.
    void yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym);

    /// Push a new look ahead token on the state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param s    the state
    /// \param sym  the symbol (for its value and location).
    /// \warning the contents of \a sym.value is stolen.
    void yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym);

    /// Pop \a n symbols from the stack.
    void yypop_ (int n = 1);

    /// Constants.
    enum
    {
      yylast_ = 428,     ///< Last index in yytable_.
      yynnts_ = 30,  ///< Number of nonterminal symbols.
      yyfinal_ = 56 ///< Termination state number.
    };


    // User arguments.
    ffi::Lexer &lexer;
    ffi::ParsingDriver &driver;

  };


#line 13 "c.y"
} // ffi
#line 990 "yy_parser_generated.hpp"




#endif // !YY_YY_YY_PARSER_GENERATED_HPP_INCLUDED
