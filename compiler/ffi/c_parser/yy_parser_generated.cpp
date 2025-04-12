// A Bison parser, made by GNU Bison 3.7.5.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2021 Free Software Foundation, Inc.

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

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.

// "%code top" blocks.
#line 41 "c.y"

    #include "compiler/ffi/c_parser/lexer.h"
    #include "compiler/ffi/c_parser/parsing_driver.h"
    #include "compiler/ffi/c_parser/yy_parser_generated.hpp"

    #define yylex(sym, loc) lexer.get_next_token((sym), (loc))

#line 47 "yy_parser_generated.cpp"




#include "yy_parser_generated.hpp"




#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif


// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K].location)
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

# ifndef YYLLOC_DEFAULT
#  define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).begin  = YYRHSLOC (Rhs, 1).begin;                   \
          (Current).end    = YYRHSLOC (Rhs, N).end;                     \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;      \
        }                                                               \
    while (false)
# endif


// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yy_stack_print_ ();                \
  } while (false)

#else // !YYDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YY_USE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

#line 13 "c.y"
namespace ffi {
#line 147 "yy_parser_generated.cpp"

  /// Build a parser object.
  YYParser::YYParser (ffi::Lexer &lexer_yyarg, ffi::ParsingDriver &driver_yyarg)
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      lexer (lexer_yyarg),
      driver (driver_yyarg)
  {}

  YYParser::~YYParser ()
  {}

  YYParser::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------------.
  | symbol kinds.  |
  `---------------*/

  // basic_symbol.
  template <typename Base>
  YYParser::basic_symbol<Base>::basic_symbol (const basic_symbol& that)
    : Base (that)
    , value (that.value)
    , location (that.location)
  {}


  /// Constructor for valueless symbols.
  template <typename Base>
  YYParser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, YY_MOVE_REF (location_type) l)
    : Base (t)
    , value ()
    , location (l)
  {}

  template <typename Base>
  YYParser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, YY_RVREF (semantic_type) v, YY_RVREF (location_type) l)
    : Base (t)
    , value (YY_MOVE (v))
    , location (YY_MOVE (l))
  {}

  template <typename Base>
  YYParser::symbol_kind_type
  YYParser::basic_symbol<Base>::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }

  template <typename Base>
  bool
  YYParser::basic_symbol<Base>::empty () const YY_NOEXCEPT
  {
    return this->kind () == symbol_kind::S_YYEMPTY;
  }

  template <typename Base>
  void
  YYParser::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move (s);
    value = YY_MOVE (s.value);
    location = YY_MOVE (s.location);
  }

  // by_kind.
  YYParser::by_kind::by_kind ()
    : kind_ (symbol_kind::S_YYEMPTY)
  {}

#if 201103L <= YY_CPLUSPLUS
  YYParser::by_kind::by_kind (by_kind&& that)
    : kind_ (that.kind_)
  {
    that.clear ();
  }
#endif

  YYParser::by_kind::by_kind (const by_kind& that)
    : kind_ (that.kind_)
  {}

  YYParser::by_kind::by_kind (token_kind_type t)
    : kind_ (yytranslate_ (t))
  {}

  void
  YYParser::by_kind::clear () YY_NOEXCEPT
  {
    kind_ = symbol_kind::S_YYEMPTY;
  }

  void
  YYParser::by_kind::move (by_kind& that)
  {
    kind_ = that.kind_;
    that.clear ();
  }

  YYParser::symbol_kind_type
  YYParser::by_kind::kind () const YY_NOEXCEPT
  {
    return kind_;
  }

  YYParser::symbol_kind_type
  YYParser::by_kind::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }


  // by_state.
  YYParser::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  YYParser::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  YYParser::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  YYParser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  YYParser::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  YYParser::symbol_kind_type
  YYParser::by_state::kind () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return symbol_kind::S_YYEMPTY;
    else
      return YY_CAST (symbol_kind_type, yystos_[+state]);
  }

  YYParser::stack_symbol_type::stack_symbol_type ()
  {}

  YYParser::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.value), YY_MOVE (that.location))
  {
#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  YYParser::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.value), YY_MOVE (that.location))
  {
    // that is emptied.
    that.kind_ = symbol_kind::S_YYEMPTY;
  }

#if YY_CPLUSPLUS < 201103L
  YYParser::stack_symbol_type&
  YYParser::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    value = that.value;
    location = that.location;
    return *this;
  }

  YYParser::stack_symbol_type&
  YYParser::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    value = that.value;
    location = that.location;
    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
  YYParser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);

    // User destructor.
    YY_USE (yysym.kind ());
  }

#if YYDEBUG
  template <typename Base>
  void
  YYParser::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YY_USE (yyoutput);
    if (yysym.empty ())
      yyo << "empty symbol";
    else
      {
        symbol_kind_type yykind = yysym.kind ();
        yyo << (yykind < YYNTOKENS ? "token" : "nterm")
            << ' ' << yysym.name () << " ("
            << yysym.location << ": ";
        YY_USE (yykind);
        yyo << ')';
      }
  }
#endif

  void
  YYParser::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  YYParser::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  YYParser::yypop_ (int n)
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  YYParser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  YYParser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  YYParser::debug_level_type
  YYParser::debug_level () const
  {
    return yydebug_;
  }

  void
  YYParser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  YYParser::state_type
  YYParser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - YYNTOKENS];
  }

  bool
  YYParser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  YYParser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
  }

  int
  YYParser::operator() ()
  {
    return parse ();
  }

  int
  YYParser::parse ()
  {
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The locations where the error started and ended.
    stack_symbol_type yyerror_range[3];

    /// The return value of parse ().
    int yyresult;

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

  /*-----------------------------------------------.
  | yynewstate -- push a new symbol on the stack.  |
  `-----------------------------------------------*/
  yynewstate:
    YYCDEBUG << "Entering state " << int (yystack_[0].state) << '\n';
    YY_STACK_PRINT ();

    // Accept?
    if (yystack_[0].state == yyfinal_)
      YYACCEPT;

    goto yybackup;


  /*-----------.
  | yybackup.  |
  `-----------*/
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[+yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token\n";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            yyla.kind_ = yytranslate_ (yylex (&yyla.value, &yyla.location));
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    if (yyla.kind () == symbol_kind::S_YYerror)
    {
      // The scanner already issued an error message, process directly
      // to error recovery.  But do not keep the error token as
      // lookahead, it is too special and may lead us to an endless
      // loop in error recovery. */
      yyla.kind_ = symbol_kind::S_YYUNDEF;
      goto yyerrlab1;
    }

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.kind ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.kind ())
      {
        goto yydefault;
      }

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", state_type (yyn), YY_MOVE (yyla));
    goto yynewstate;


  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[+yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;


  /*-----------------------------.
  | yyreduce -- do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* If YYLEN is nonzero, implement the default value of the
         action: '$$ = $1'.  Otherwise, use the top of the stack.

         Otherwise, the following line sets YYLHS.VALUE to garbage.
         This behavior is undocumented and Bison users should not rely
         upon it.  */
      if (yylen)
        yylhs.value = yystack_[yylen - 1].value;
      else
        yylhs.value = yystack_[0].value;

      // Default location.
      {
        stack_type::slice range (yystack_, yylen);
        YYLLOC_DEFAULT (yylhs.location, range, yylen);
        yyerror_range[1].location = yylhs.location;
      }

      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 4: // external_declaration: declaration
#line 130 "c.y"
                { driver.add_type((yystack_[0].value. type_ )); }
#line 615 "yy_parser_generated.cpp"
    break;

  case 5: // external_declaration: typedef_declaration
#line 131 "c.y"
                        {}
#line 621 "yy_parser_generated.cpp"
    break;

  case 6: // external_declaration: EXTERN declaration
#line 132 "c.y"
                       { driver.add_type((yystack_[0].value. type_ )); }
#line 627 "yy_parser_generated.cpp"
    break;

  case 7: // external_declaration: STATIC declaration
#line 133 "c.y"
                       { driver.add_type((yystack_[0].value. type_ )); }
#line 633 "yy_parser_generated.cpp"
    break;

  case 8: // external_declaration: AUTO declaration
#line 134 "c.y"
                     { driver.add_type((yystack_[0].value. type_ )); }
#line 639 "yy_parser_generated.cpp"
    break;

  case 9: // external_declaration: REGISTER declaration
#line 135 "c.y"
                         { driver.add_type((yystack_[0].value. type_ )); }
#line 645 "yy_parser_generated.cpp"
    break;

  case 10: // typedef_declaration: TYPEDEF specifier_qualifier_list declarator ";"
#line 139 "c.y"
                                                          { driver.add_typedef((yystack_[2].value. type_ ), (yystack_[1].value. type_ )); }
#line 651 "yy_parser_generated.cpp"
    break;

  case 11: // declaration: declaration_specifiers ";"
#line 143 "c.y"
                                     { (yylhs.value. type_ ) = (yystack_[1].value. type_ ); }
#line 657 "yy_parser_generated.cpp"
    break;

  case 12: // declaration: declaration_specifiers init_declarator_list ";"
#line 144 "c.y"
                                                          { (yylhs.value. type_ ) = driver.combine(DeclarationSpecifiers{(yystack_[2].value. type_ )}, InitDeclaratorList{(yystack_[1].value. type_ )}); }
#line 663 "yy_parser_generated.cpp"
    break;

  case 13: // declaration_specifiers: type_specifier
#line 148 "c.y"
                   { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 669 "yy_parser_generated.cpp"
    break;

  case 14: // declaration_specifiers: type_specifier declaration_specifiers
#line 149 "c.y"
                                          { (yylhs.value. type_ ) = driver.combine(TypeSpecifier{(yystack_[1].value. type_ )}, DeclarationSpecifiers{(yystack_[0].value. type_ )}); }
#line 675 "yy_parser_generated.cpp"
    break;

  case 15: // declaration_specifiers: type_qualifier
#line 150 "c.y"
                   { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 681 "yy_parser_generated.cpp"
    break;

  case 16: // declaration_specifiers: type_qualifier declaration_specifiers
#line 151 "c.y"
                                          { (yylhs.value. type_ ) = driver.combine(TypeQualifier{(yystack_[1].value. type_ )}, DeclarationSpecifiers{(yystack_[0].value. type_ )}); }
#line 687 "yy_parser_generated.cpp"
    break;

  case 17: // type_qualifier: CONST
#line 155 "c.y"
          { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Const); }
#line 693 "yy_parser_generated.cpp"
    break;

  case 18: // type_qualifier: VOLATILE
#line 156 "c.y"
             { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Volatile); }
#line 699 "yy_parser_generated.cpp"
    break;

  case 19: // base_type_specifier: VOID
#line 160 "c.y"
         { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Void); }
#line 705 "yy_parser_generated.cpp"
    break;

  case 20: // base_type_specifier: CHAR
#line 161 "c.y"
         { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Char); }
#line 711 "yy_parser_generated.cpp"
    break;

  case 21: // base_type_specifier: BOOL
#line 162 "c.y"
         { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Bool); }
#line 717 "yy_parser_generated.cpp"
    break;

  case 22: // base_type_specifier: FLOAT
#line 163 "c.y"
          { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Float); }
#line 723 "yy_parser_generated.cpp"
    break;

  case 23: // base_type_specifier: DOUBLE
#line 164 "c.y"
           { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Double); }
#line 729 "yy_parser_generated.cpp"
    break;

  case 24: // base_type_specifier: INT
#line 165 "c.y"
        { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_int); }
#line 735 "yy_parser_generated.cpp"
    break;

  case 25: // base_type_specifier: LONG
#line 166 "c.y"
         { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Long); }
#line 741 "yy_parser_generated.cpp"
    break;

  case 26: // base_type_specifier: SHORT
#line 167 "c.y"
          { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Short); }
#line 747 "yy_parser_generated.cpp"
    break;

  case 27: // base_type_specifier: SIGNED
#line 168 "c.y"
           { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Signed); }
#line 753 "yy_parser_generated.cpp"
    break;

  case 28: // base_type_specifier: UNSIGNED
#line 169 "c.y"
             { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Unsigned); }
#line 759 "yy_parser_generated.cpp"
    break;

  case 29: // base_type_specifier: INT8
#line 170 "c.y"
         { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Int8); }
#line 765 "yy_parser_generated.cpp"
    break;

  case 30: // base_type_specifier: INT16
#line 171 "c.y"
          { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Int16); }
#line 771 "yy_parser_generated.cpp"
    break;

  case 31: // base_type_specifier: INT32
#line 172 "c.y"
          { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Int32); }
#line 777 "yy_parser_generated.cpp"
    break;

  case 32: // base_type_specifier: INT64
#line 173 "c.y"
          { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Int64); }
#line 783 "yy_parser_generated.cpp"
    break;

  case 33: // base_type_specifier: UINT8
#line 174 "c.y"
          { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Uint8); }
#line 789 "yy_parser_generated.cpp"
    break;

  case 34: // base_type_specifier: UINT16
#line 175 "c.y"
           { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Uint16); }
#line 795 "yy_parser_generated.cpp"
    break;

  case 35: // base_type_specifier: UINT32
#line 176 "c.y"
           { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Uint32); }
#line 801 "yy_parser_generated.cpp"
    break;

  case 36: // base_type_specifier: UINT64
#line 177 "c.y"
           { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Uint64); }
#line 807 "yy_parser_generated.cpp"
    break;

  case 37: // base_type_specifier: INTPTR_T
#line 178 "c.y"
             { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_intptr); }
#line 813 "yy_parser_generated.cpp"
    break;

  case 38: // base_type_specifier: UINTPTR_T
#line 179 "c.y"
              { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_uintptr); }
#line 819 "yy_parser_generated.cpp"
    break;

  case 39: // base_type_specifier: SIZE_T
#line 180 "c.y"
           { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_size); }
#line 825 "yy_parser_generated.cpp"
    break;

  case 40: // type_specifier: base_type_specifier
#line 184 "c.y"
                        { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 831 "yy_parser_generated.cpp"
    break;

  case 41: // type_specifier: struct_or_union_specifier
#line 185 "c.y"
                              { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 837 "yy_parser_generated.cpp"
    break;

  case 42: // type_specifier: enum_specifier
#line 186 "c.y"
                   { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 843 "yy_parser_generated.cpp"
    break;

  case 43: // type_specifier: TYPEDEF_NAME
#line 187 "c.y"
                 { (yylhs.value. type_ ) = driver.get_aliased_type((yystack_[0].value. str_ )); }
#line 849 "yy_parser_generated.cpp"
    break;

  case 44: // init_declarator_list: init_declarator
#line 191 "c.y"
                    { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_initDeclaratorList); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 855 "yy_parser_generated.cpp"
    break;

  case 45: // init_declarator_list: init_declarator_list "," init_declarator
#line 192 "c.y"
                                               { (yylhs.value. type_ ) = (yystack_[2].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 861 "yy_parser_generated.cpp"
    break;

  case 46: // init_declarator: declarator
#line 197 "c.y"
               { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 867 "yy_parser_generated.cpp"
    break;

  case 47: // declarator: direct_declarator
#line 201 "c.y"
                      { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 873 "yy_parser_generated.cpp"
    break;

  case 48: // declarator: pointer direct_declarator
#line 202 "c.y"
                              { (yylhs.value. type_ ) = driver.combine(Pointer{(yystack_[1].value. type_ )}, DirectDeclarator{(yystack_[0].value. type_ )}); }
#line 879 "yy_parser_generated.cpp"
    break;

  case 49: // pointer: "*" type_qualifier_list pointer
#line 208 "c.y"
                                    { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); (yylhs.value. type_ )->num++; }
#line 885 "yy_parser_generated.cpp"
    break;

  case 50: // pointer: "*" type_qualifier_list
#line 209 "c.y"
                            { (yylhs.value. type_ ) = driver.make_pointer(); }
#line 891 "yy_parser_generated.cpp"
    break;

  case 51: // pointer: "*" pointer
#line 210 "c.y"
                { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); (yylhs.value. type_ )->num++; }
#line 897 "yy_parser_generated.cpp"
    break;

  case 52: // pointer: "*"
#line 211 "c.y"
        { (yylhs.value. type_ ) = driver.make_pointer(); }
#line 903 "yy_parser_generated.cpp"
    break;

  case 53: // type_qualifier_list: type_qualifier
#line 215 "c.y"
                   { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typesList); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 909 "yy_parser_generated.cpp"
    break;

  case 54: // type_qualifier_list: type_qualifier_list type_qualifier
#line 216 "c.y"
                                       { (yylhs.value. type_ ) = (yystack_[1].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 915 "yy_parser_generated.cpp"
    break;

  case 55: // direct_declarator: IDENTIFIER
#line 220 "c.y"
               { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Name); (yylhs.value. type_ )->str = (yystack_[0].value. str_ ).to_string(); }
#line 921 "yy_parser_generated.cpp"
    break;

  case 56: // direct_declarator: "(" declarator ")"
#line 221 "c.y"
                             { (yylhs.value. type_ ) = (yystack_[1].value. type_ ); }
#line 927 "yy_parser_generated.cpp"
    break;

  case 57: // direct_declarator: direct_declarator "[" "]"
#line 222 "c.y"
                                        { (yylhs.value. type_ ) = driver.make_array_declarator((yystack_[2].value. type_ ), "-1"); }
#line 933 "yy_parser_generated.cpp"
    break;

  case 58: // direct_declarator: direct_declarator "[" INT_CONSTANT "]"
#line 223 "c.y"
                                                     { (yylhs.value. type_ ) = driver.make_array_declarator((yystack_[3].value. type_ ), (yystack_[1].value. str_ )); }
#line 939 "yy_parser_generated.cpp"
    break;

  case 59: // direct_declarator: direct_declarator "(" parameter_type_list ")"
#line 224 "c.y"
                                                        { (yylhs.value. type_ ) = driver.make_function((yystack_[3].value. type_ ), (yystack_[1].value. type_ )); }
#line 945 "yy_parser_generated.cpp"
    break;

  case 60: // direct_declarator: direct_declarator "(" ")"
#line 225 "c.y"
                                    { (yylhs.value. type_ ) = driver.make_function((yystack_[2].value. type_ ), nullptr); }
#line 951 "yy_parser_generated.cpp"
    break;

  case 61: // parameter_type_list: parameter_list "," ELLIPSIS
#line 229 "c.y"
                                  { (yylhs.value. type_ ) = (yystack_[2].value. type_ ); (yylhs.value. type_ )->members.emplace_back(driver.make_simple_type(FFITypeKind::_ellipsis)); }
#line 957 "yy_parser_generated.cpp"
    break;

  case 62: // parameter_type_list: parameter_list
#line 230 "c.y"
                   { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 963 "yy_parser_generated.cpp"
    break;

  case 63: // parameter_list: parameter_declaration
#line 234 "c.y"
                          { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typesList); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 969 "yy_parser_generated.cpp"
    break;

  case 64: // parameter_list: parameter_list "," parameter_declaration
#line 235 "c.y"
                                               { (yylhs.value. type_ ) = (yystack_[2].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 975 "yy_parser_generated.cpp"
    break;

  case 65: // parameter_declaration: declaration_specifiers declarator
#line 239 "c.y"
                                      { (yylhs.value. type_ ) = driver.combine(DeclarationSpecifiers{(yystack_[1].value. type_ )}, Declarator{(yystack_[0].value. type_ )}); }
#line 981 "yy_parser_generated.cpp"
    break;

  case 66: // parameter_declaration: declaration_specifiers abstract_declarator
#line 240 "c.y"
                                               { (yylhs.value. type_ ) = driver.combine(DeclarationSpecifiers{(yystack_[1].value. type_ )}, AbstractDeclarator{(yystack_[0].value. type_ )}); }
#line 987 "yy_parser_generated.cpp"
    break;

  case 67: // parameter_declaration: declaration_specifiers
#line 241 "c.y"
                           { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 993 "yy_parser_generated.cpp"
    break;

  case 68: // abstract_declarator: pointer direct_abstract_declarator
#line 245 "c.y"
                                       { (yylhs.value. type_ ) = driver.combine(Pointer{(yystack_[1].value. type_ )}, DirectAbstractDeclarator{(yystack_[0].value. type_ )}); }
#line 999 "yy_parser_generated.cpp"
    break;

  case 69: // abstract_declarator: pointer
#line 246 "c.y"
            { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 1005 "yy_parser_generated.cpp"
    break;

  case 70: // abstract_declarator: direct_abstract_declarator
#line 247 "c.y"
                               { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 1011 "yy_parser_generated.cpp"
    break;

  case 71: // direct_abstract_declarator: "[" "]"
#line 251 "c.y"
                      { (yylhs.value. type_ ) = driver.make_abstract_array_declarator("-1"); }
#line 1017 "yy_parser_generated.cpp"
    break;

  case 72: // direct_abstract_declarator: "[" INT_CONSTANT "]"
#line 252 "c.y"
                                   { (yylhs.value. type_ ) = driver.make_abstract_array_declarator((yystack_[1].value. str_ )); }
#line 1023 "yy_parser_generated.cpp"
    break;

  case 73: // direct_abstract_declarator: direct_abstract_declarator "[" "]"
#line 253 "c.y"
                                                 { (yylhs.value. type_ ) = driver.make_array_declarator((yystack_[2].value. type_ ), "-1"); }
#line 1029 "yy_parser_generated.cpp"
    break;

  case 74: // direct_abstract_declarator: direct_abstract_declarator "[" INT_CONSTANT "]"
#line 254 "c.y"
                                                              { (yylhs.value. type_ ) = driver.make_array_declarator((yystack_[3].value. type_ ), (yystack_[1].value. str_ )); }
#line 1035 "yy_parser_generated.cpp"
    break;

  case 75: // struct_or_union_specifier: STRUCT "{" struct_declaration_list "}"
#line 258 "c.y"
                                                 { (yylhs.value. type_ ) = driver.make_struct_def((yystack_[1].value. type_ )); }
#line 1041 "yy_parser_generated.cpp"
    break;

  case 76: // struct_or_union_specifier: STRUCT typedef_name_or_identifier "{" struct_declaration_list "}"
#line 259 "c.y"
                                                                            { (yylhs.value. type_ ) = driver.make_struct_def((yystack_[1].value. type_ )); (yylhs.value. type_ )->str = (yystack_[3].value. str_ ).to_string(); }
#line 1047 "yy_parser_generated.cpp"
    break;

  case 77: // struct_or_union_specifier: STRUCT typedef_name_or_identifier
#line 260 "c.y"
                                      { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Struct); (yylhs.value. type_ )->str = (yystack_[0].value. str_ ).to_string(); }
#line 1053 "yy_parser_generated.cpp"
    break;

  case 78: // struct_or_union_specifier: UNION "{" struct_declaration_list "}"
#line 261 "c.y"
                                                { (yylhs.value. type_ ) = driver.make_union_def((yystack_[1].value. type_ )); }
#line 1059 "yy_parser_generated.cpp"
    break;

  case 79: // struct_or_union_specifier: UNION typedef_name_or_identifier "{" struct_declaration_list "}"
#line 262 "c.y"
                                                                           { (yylhs.value. type_ ) = driver.make_union_def((yystack_[1].value. type_ )); (yylhs.value. type_ )->str = (yystack_[3].value. str_ ).to_string(); }
#line 1065 "yy_parser_generated.cpp"
    break;

  case 80: // struct_or_union_specifier: UNION typedef_name_or_identifier
#line 263 "c.y"
                                     { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Union); (yylhs.value. type_ )->str = (yystack_[0].value. str_ ).to_string(); }
#line 1071 "yy_parser_generated.cpp"
    break;

  case 81: // typedef_name_or_identifier: IDENTIFIER
#line 267 "c.y"
               { (yylhs.value. str_ ) = (yystack_[0].value. str_ ); }
#line 1077 "yy_parser_generated.cpp"
    break;

  case 82: // typedef_name_or_identifier: TYPEDEF_NAME
#line 268 "c.y"
                 { (yylhs.value. str_ ) = (yystack_[0].value. str_ ); }
#line 1083 "yy_parser_generated.cpp"
    break;

  case 83: // struct_declaration_list: struct_declaration
#line 272 "c.y"
                       { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typesList); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 1089 "yy_parser_generated.cpp"
    break;

  case 84: // struct_declaration_list: struct_declaration_list struct_declaration
#line 273 "c.y"
                                               { (yylhs.value. type_ ) = (yystack_[1].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 1095 "yy_parser_generated.cpp"
    break;

  case 85: // struct_declaration: specifier_qualifier_list struct_declarator_list ";"
#line 277 "c.y"
                                                              { (yylhs.value. type_ ) = (yystack_[1].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[2].value. type_ )); }
#line 1101 "yy_parser_generated.cpp"
    break;

  case 86: // specifier_qualifier_list: type_specifier specifier_qualifier_list
#line 281 "c.y"
                                            { (yylhs.value. type_ ) = driver.combine(TypeSpecifier{(yystack_[1].value. type_ )}, SpecifierQualifierList{(yystack_[0].value. type_ )}); }
#line 1107 "yy_parser_generated.cpp"
    break;

  case 87: // specifier_qualifier_list: type_specifier
#line 282 "c.y"
                   { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 1113 "yy_parser_generated.cpp"
    break;

  case 88: // specifier_qualifier_list: type_qualifier specifier_qualifier_list
#line 283 "c.y"
                                            { (yylhs.value. type_ ) = driver.combine(TypeQualifier{(yystack_[1].value. type_ )}, SpecifierQualifierList{(yystack_[0].value. type_ )}); }
#line 1119 "yy_parser_generated.cpp"
    break;

  case 89: // specifier_qualifier_list: type_qualifier
#line 284 "c.y"
                   { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 1125 "yy_parser_generated.cpp"
    break;

  case 90: // struct_declarator_list: struct_declarator
#line 288 "c.y"
                      { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_structFieldList); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 1131 "yy_parser_generated.cpp"
    break;

  case 91: // struct_declarator_list: struct_declarator_list "," struct_declarator
#line 289 "c.y"
                                                   { (yylhs.value. type_ ) = (yystack_[2].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 1137 "yy_parser_generated.cpp"
    break;

  case 92: // struct_declarator: declarator
#line 293 "c.y"
               { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 1143 "yy_parser_generated.cpp"
    break;

  case 93: // enum_specifier: ENUM "{" enumerator_list "}"
#line 300 "c.y"
                                       { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::EnumDef); }
#line 1149 "yy_parser_generated.cpp"
    break;

  case 94: // enum_specifier: ENUM "{" enumerator_list "," "}"
#line 301 "c.y"
                                             { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::EnumDef); }
#line 1155 "yy_parser_generated.cpp"
    break;

  case 95: // enum_specifier: ENUM IDENTIFIER "{" enumerator_list "}"
#line 302 "c.y"
                                                  { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::EnumDef); (yylhs.value. type_ )->str = (yystack_[3].value. str_ ).to_string(); }
#line 1161 "yy_parser_generated.cpp"
    break;

  case 96: // enum_specifier: ENUM IDENTIFIER "{" enumerator_list "," "}"
#line 303 "c.y"
                                                        { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::EnumDef); (yylhs.value. type_ )->str = (yystack_[4].value. str_ ).to_string(); }
#line 1167 "yy_parser_generated.cpp"
    break;

  case 97: // enum_specifier: ENUM IDENTIFIER
#line 304 "c.y"
                    { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Enum); (yylhs.value. type_ )->str = (yystack_[0].value. str_ ).to_string(); }
#line 1173 "yy_parser_generated.cpp"
    break;

  case 98: // enumerator_list: enumerator
#line 308 "c.y"
               { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_enumList); driver.add_enumerator((yylhs.value. type_ ), (yystack_[0].value. type_ )); }
#line 1179 "yy_parser_generated.cpp"
    break;

  case 99: // enumerator_list: enumerator_list "," enumerator
#line 309 "c.y"
                                     { (yylhs.value. type_ ) = (yystack_[2].value. type_ ); driver.add_enumerator((yylhs.value. type_ ), (yystack_[0].value. type_ )); }
#line 1185 "yy_parser_generated.cpp"
    break;

  case 100: // enumerator: IDENTIFIER "=" int_value
#line 313 "c.y"
                               { (yylhs.value. type_ ) = driver.make_enum_member((yystack_[2].value. str_ ), (yystack_[0].value. int_ )); }
#line 1191 "yy_parser_generated.cpp"
    break;

  case 101: // enumerator: IDENTIFIER
#line 314 "c.y"
               { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_enumMember); (yylhs.value. type_ )->str = (yystack_[0].value. str_ ).to_string(); }
#line 1197 "yy_parser_generated.cpp"
    break;

  case 102: // int_value: INT_CONSTANT
#line 318 "c.y"
                 { (yylhs.value. int_ ) = driver.stoi((yystack_[0].value. str_ )); }
#line 1203 "yy_parser_generated.cpp"
    break;

  case 103: // int_value: "-" INT_CONSTANT
#line 319 "c.y"
                       { (yylhs.value. int_ ) = -driver.stoi((yystack_[0].value. str_ )); }
#line 1209 "yy_parser_generated.cpp"
    break;

  case 104: // int_value: "+" INT_CONSTANT
#line 320 "c.y"
                      { (yylhs.value. int_ ) = driver.stoi((yystack_[0].value. str_ )); }
#line 1215 "yy_parser_generated.cpp"
    break;


#line 1219 "yy_parser_generated.cpp"

            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;


  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        context yyctx (*this, yyla);
        std::string msg = yysyntax_error_ (yyctx);
        error (yyla.location, YY_MOVE (msg));
      }


    yyerror_range[1].location = yyla.location;
    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.kind () == symbol_kind::S_YYEOF)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:
    /* Pacify compilers when the user code never invokes YYERROR and
       the label yyerrorlab therefore never appears in user code.  */
    if (false)
      YYERROR;

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    YY_STACK_PRINT ();
    goto yyerrlab1;


  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    // Pop stack until we find a state that shifts the error token.
    for (;;)
      {
        yyn = yypact_[+yystack_[0].state];
        if (!yy_pact_value_is_default_ (yyn))
          {
            yyn += symbol_kind::S_YYerror;
            if (0 <= yyn && yyn <= yylast_
                && yycheck_[yyn] == symbol_kind::S_YYerror)
              {
                yyn = yytable_[yyn];
                if (0 < yyn)
                  break;
              }
          }

        // Pop the current state because it cannot handle the error token.
        if (yystack_.size () == 1)
          YYABORT;

        yyerror_range[1].location = yystack_[0].location;
        yy_destroy_ ("Error: popping", yystack_[0]);
        yypop_ ();
        YY_STACK_PRINT ();
      }
    {
      stack_symbol_type error_token;

      yyerror_range[2].location = yyla.location;
      YYLLOC_DEFAULT (error_token.location, yyerror_range, 2);

      // Shift the error token.
      error_token.state = state_type (yyn);
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;


  /*-------------------------------------.
  | yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;


  /*-----------------------------------.
  | yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
  yyabortlab:
    yyresult = 1;
    goto yyreturn;


  /*-----------------------------------------------------.
  | yyreturn -- parsing is finished, return the result.  |
  `-----------------------------------------------------*/
  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    YY_STACK_PRINT ();
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
  YYParser::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
  YYParser::yytnamerr_ (const char *yystr)
  {
    if (*yystr == '"')
      {
        std::string yyr;
        char const *yyp = yystr;

        for (;;)
          switch (*++yyp)
            {
            case '\'':
            case ',':
              goto do_not_strip_quotes;

            case '\\':
              if (*++yyp != '\\')
                goto do_not_strip_quotes;
              else
                goto append;

            append:
            default:
              yyr += *yyp;
              break;

            case '"':
              return yyr;
            }
      do_not_strip_quotes: ;
      }

    return yystr;
  }

  std::string
  YYParser::symbol_name (symbol_kind_type yysymbol)
  {
    return yytnamerr_ (yytname_[yysymbol]);
  }



  // YYParser::context.
  YYParser::context::context (const YYParser& yyparser, const symbol_type& yyla)
    : yyparser_ (yyparser)
    , yyla_ (yyla)
  {}

  int
  YYParser::context::expected_tokens (symbol_kind_type yyarg[], int yyargn) const
  {
    // Actual number of expected tokens
    int yycount = 0;

    int yyn = yypact_[+yyparser_.yystack_[0].state];
    if (!yy_pact_value_is_default_ (yyn))
      {
        /* Start YYX at -YYN if negative to avoid negative indexes in
           YYCHECK.  In other words, skip the first -YYN actions for
           this state because they are default actions.  */
        int yyxbegin = yyn < 0 ? -yyn : 0;
        // Stay within bounds of both yycheck and yytname.
        int yychecklim = yylast_ - yyn + 1;
        int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
        for (int yyx = yyxbegin; yyx < yyxend; ++yyx)
          if (yycheck_[yyx + yyn] == yyx && yyx != symbol_kind::S_YYerror
              && !yy_table_value_is_error_ (yytable_[yyx + yyn]))
            {
              if (!yyarg)
                ++yycount;
              else if (yycount == yyargn)
                return 0;
              else
                yyarg[yycount++] = YY_CAST (symbol_kind_type, yyx);
            }
      }

    if (yyarg && yycount == 0 && 0 < yyargn)
      yyarg[0] = symbol_kind::S_YYEMPTY;
    return yycount;
  }



  int
  YYParser::yy_syntax_error_arguments_ (const context& yyctx,
                                                 symbol_kind_type yyarg[], int yyargn) const
  {
    /* There are many possibilities here to consider:
       - If this state is a consistent state with a default action, then
         the only way this function was invoked is if the default action
         is an error action.  In that case, don't check for expected
         tokens because there are none.
       - The only way there can be no lookahead present (in yyla) is
         if this state is a consistent state with a default action.
         Thus, detecting the absence of a lookahead is sufficient to
         determine that there is no unexpected or expected token to
         report.  In that case, just report a simple "syntax error".
       - Don't assume there isn't a lookahead just because this state is
         a consistent state with a default action.  There might have
         been a previous inconsistent state, consistent state with a
         non-default action, or user semantic action that manipulated
         yyla.  (However, yyla is currently not documented for users.)
       - Of course, the expected token list depends on states to have
         correct lookahead information, and it depends on the parser not
         to perform extra reductions after fetching a lookahead from the
         scanner and before detecting a syntax error.  Thus, state merging
         (from LALR or IELR) and default reductions corrupt the expected
         token list.  However, the list is correct for canonical LR with
         one exception: it will still contain any token that will not be
         accepted due to an error action in a later state.
    */

    if (!yyctx.lookahead ().empty ())
      {
        if (yyarg)
          yyarg[0] = yyctx.token ();
        int yyn = yyctx.expected_tokens (yyarg ? yyarg + 1 : yyarg, yyargn - 1);
        return yyn + 1;
      }
    return 0;
  }

  // Generate an error message.
  std::string
  YYParser::yysyntax_error_ (const context& yyctx) const
  {
    // Its maximum.
    enum { YYARGS_MAX = 5 };
    // Arguments of yyformat.
    symbol_kind_type yyarg[YYARGS_MAX];
    int yycount = yy_syntax_error_arguments_ (yyctx, yyarg, YYARGS_MAX);

    char const* yyformat = YY_NULLPTR;
    switch (yycount)
      {
#define YYCASE_(N, S)                         \
        case N:                               \
          yyformat = S;                       \
        break
      default: // Avoid compiler warnings.
        YYCASE_ (0, YY_("syntax error"));
        YYCASE_ (1, YY_("syntax error, unexpected %s"));
        YYCASE_ (2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_ (3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_ (4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_ (5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
      }

    std::string yyres;
    // Argument number.
    std::ptrdiff_t yyi = 0;
    for (char const* yyp = yyformat; *yyp; ++yyp)
      if (yyp[0] == '%' && yyp[1] == 's' && yyi < yycount)
        {
          yyres += symbol_name (yyarg[yyi++]);
          ++yyp;
        }
      else
        yyres += *yyp;
    return yyres;
  }


  const signed char YYParser::yypact_ninf_ = -78;

  const signed char YYParser::yytable_ninf_ = -1;

  const short
  YYParser::yypact_[] =
  {
     332,   -78,   366,   366,   366,   366,   366,   -78,   -78,   -78,
     -78,   -78,   -78,   -78,   -78,   -78,   -78,   -78,   -78,   -78,
     -78,   -78,   -78,   -78,   -78,   -78,   -78,   -78,   -78,   -78,
      10,    13,    16,    56,   -78,   -78,   -78,    67,   366,   -78,
     366,   -78,   -78,   366,   366,    65,   -78,   -78,   -78,   -78,
     366,   -78,   -78,    31,   366,    44,    50,    76,   -78,   -78,
     -78,    65,     5,   -78,    19,   -78,   -78,    22,   132,   -78,
     -78,   -78,   -78,    66,   125,   -78,    65,   366,   159,   366,
      84,    83,   -78,    50,    80,   -78,   -78,     5,    65,   -78,
     132,   263,    42,   -78,   -78,   -78,   -78,   141,   -78,   195,
     -78,   229,   120,    24,   -78,   134,   -78,   -78,   -78,   -78,
     -78,    46,   124,   135,   -78,   -78,   136,    65,   -78,   -78,
     -78,   126,   131,   -78,   -78,   -78,   -78,    28,   -78,    69,
     -78,    68,   -78,   139,   -78,   297,   -78,   -78,   -78,   -78,
     -78,   -78,   140,   139,    74,   -78,   -78,   -78,   -78,   142,
     -78
  };

  const signed char
  YYParser::yydefact_[] =
  {
       0,    43,     0,     0,     0,     0,     0,    21,    20,    26,
      24,    25,    27,    28,    22,    23,    17,    18,    19,    29,
      30,    31,    32,    33,    34,    35,    36,    39,    37,    38,
       0,     0,     0,     0,     2,     5,     4,     0,    15,    40,
      13,    41,    42,    89,    87,     0,     6,     7,     8,     9,
       0,    81,    82,    77,     0,    80,     0,    97,     1,     3,
      11,     0,    52,    55,     0,    44,    46,     0,    47,    16,
      14,    88,    86,     0,     0,    83,     0,     0,     0,     0,
     101,     0,    98,     0,     0,    53,    51,    50,     0,    12,
      48,     0,     0,    10,    75,    84,    92,     0,    90,     0,
      78,     0,     0,     0,    93,     0,    56,    54,    49,    45,
      60,    67,     0,    62,    63,    57,     0,     0,    85,    76,
      79,     0,     0,   102,   100,    94,    99,     0,    95,     0,
      65,    69,    66,    70,    59,     0,    58,    91,   104,   103,
      96,    71,     0,    68,     0,    61,    64,    72,    73,     0,
      74
  };

  const signed char
  YYParser::yypgoto_[] =
  {
     -78,   -78,   116,   -78,    89,   -31,     0,   -78,     8,   -78,
      63,   -44,   -52,   -78,   -65,   -78,   -78,    18,   -78,    23,
     -78,   127,   -35,   -54,    25,   -78,    38,   -78,    73,   -77,
     -78
  };

  const unsigned char
  YYParser::yydefgoto_[] =
  {
       0,    33,    34,    35,    36,    37,    43,    39,    44,    64,
      65,    66,    67,    87,    68,   112,   113,   114,   132,   133,
      41,    53,    74,    75,    76,    97,    98,    42,    81,    82,
     124
  };

  const unsigned char
  YYParser::yytable_[] =
  {
      38,    73,    90,    38,    38,    38,    38,    69,    40,    70,
      86,    40,    40,    40,    40,    50,    62,    84,    54,    78,
      95,    56,    88,    89,    95,    51,   126,    45,    51,    61,
     125,    57,    96,    38,   140,   108,    77,    63,    38,    80,
      38,    40,    99,    80,   101,    95,    40,    95,    40,    79,
     126,    52,   115,    61,    52,   129,    58,    62,   116,   131,
     111,    63,    85,    16,    17,    80,    90,   130,    71,    72,
      93,    60,    61,    96,    61,    61,    62,   129,    62,   141,
      63,    83,    63,    63,   148,   142,   103,   107,   106,   104,
     149,    38,    46,    47,    48,    49,   102,     1,     2,    40,
       3,     4,     5,     6,   111,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    94,   134,   121,   122,    38,   123,   127,   135,    91,
     128,    92,   138,    40,   117,   118,   136,   139,   144,    59,
     147,   109,   150,   146,   143,   137,   105,     0,    55,     0,
       0,     0,     0,     0,     0,   100,     1,     0,     0,     0,
       0,     0,     0,     0,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
       1,   119,     0,     0,     0,     0,     0,     0,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,     0,   120,     1,     0,     0,     0,
       0,     0,     0,     0,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
       1,   110,     0,     0,     0,     0,     0,     0,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,     1,     0,     0,     0,     0,     0,
       0,     0,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,     1,     0,
       0,     0,     0,     0,     0,     0,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,   145,     1,     2,     0,     3,     4,     5,     6,
       0,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,     1,     0,     0,
       0,     0,     0,     0,     0,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32
  };

  const short
  YYParser::yycheck_[] =
  {
       0,    45,    67,     3,     4,     5,     6,    38,     0,    40,
      62,     3,     4,     5,     6,     5,    11,    61,     5,    54,
      74,     5,     3,     4,    78,    15,   103,     2,    15,     7,
       6,    15,    76,    33,     6,    87,     5,    15,    38,    15,
      40,    33,    77,    15,    79,    99,    38,   101,    40,     5,
     127,    41,    10,     7,    41,     9,     0,    11,    16,   111,
      91,    15,    62,    58,    59,    15,   131,   111,    43,    44,
       4,     4,     7,   117,     7,     7,    11,     9,    11,    10,
      15,     5,    15,    15,    10,    16,     3,    87,     8,     6,
      16,    91,     3,     4,     5,     6,    12,    41,    42,    91,
      44,    45,    46,    47,   135,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,     6,     8,    13,    14,   135,    16,     3,     3,     7,
       6,     9,    16,   135,     3,     4,    10,    16,     9,    33,
      10,    88,    10,   135,   131,   117,    83,    -1,    31,    -1,
      -1,    -1,    -1,    -1,    -1,     6,    41,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      41,     6,    -1,    -1,    -1,    -1,    -1,    -1,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    -1,     6,    41,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      41,     8,    -1,    -1,    -1,    -1,    -1,    -1,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    41,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    41,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    41,    42,    -1,    44,    45,    46,    47,
      -1,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    41,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74
  };

  const signed char
  YYParser::yystos_[] =
  {
       0,    41,    42,    44,    45,    46,    47,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    89,    90,    91,    92,    93,    94,    95,
      96,   108,   115,    94,    96,   112,    92,    92,    92,    92,
       5,    15,    41,   109,     5,   109,     5,    15,     0,    90,
       4,     7,    11,    15,    97,    98,    99,   100,   102,    93,
      93,   112,   112,    99,   110,   111,   112,     5,   110,     5,
      15,   116,   117,     5,    99,    94,   100,   101,     3,     4,
     102,     7,     9,     4,     6,   111,    99,   113,   114,   110,
       6,   110,    12,     3,     6,   116,     8,    94,   100,    98,
       8,    93,   103,   104,   105,    10,    16,     3,     4,     6,
       6,    13,    14,    16,   118,     6,   117,     3,     6,     9,
      99,   100,   106,   107,     8,     3,    10,   114,    16,    16,
       6,    10,    16,   107,     9,    75,   105,    10,    10,    16,
      10
  };

  const signed char
  YYParser::yyr1_[] =
  {
       0,    88,    89,    89,    90,    90,    90,    90,    90,    90,
      91,    92,    92,    93,    93,    93,    93,    94,    94,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      96,    96,    96,    96,    97,    97,    98,    99,    99,   100,
     100,   100,   100,   101,   101,   102,   102,   102,   102,   102,
     102,   103,   103,   104,   104,   105,   105,   105,   106,   106,
     106,   107,   107,   107,   107,   108,   108,   108,   108,   108,
     108,   109,   109,   110,   110,   111,   112,   112,   112,   112,
     113,   113,   114,   115,   115,   115,   115,   115,   116,   116,
     117,   117,   118,   118,   118
  };

  const signed char
  YYParser::yyr2_[] =
  {
       0,     2,     1,     2,     1,     1,     2,     2,     2,     2,
       4,     2,     3,     1,     2,     1,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     1,     1,     2,     3,
       2,     2,     1,     1,     2,     1,     3,     3,     4,     4,
       3,     3,     1,     1,     3,     2,     2,     1,     2,     1,
       1,     2,     3,     3,     4,     4,     5,     2,     4,     5,
       2,     1,     1,     1,     2,     3,     2,     1,     2,     1,
       1,     3,     1,     4,     5,     5,     6,     2,     1,     3,
       3,     1,     1,     2,     2
  };


#if YYDEBUG || 1
  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a YYNTOKENS, nonterminals.
  const char*
  const YYParser::yytname_[] =
  {
  "\"end of file\"", "error", "\"invalid token\"", "\",\"", "\";\"",
  "\"{\"", "\"}\"", "\"(\"", "\")\"", "\"[\"", "\"]\"", "\"*\"", "\"=\"",
  "\"+\"", "\"-\"", "IDENTIFIER", "INT_CONSTANT", "FLOAT_CONSTANT",
  "STRING_LITERAL", "SIZEOF", "PTR_OP", "INC_OP", "DEC_OP", "LEFT_OP",
  "RIGHT_OP", "LE_OP", "GE_OP", "EQ_OP", "NE_OP", "AND_OP", "OR_OP",
  "MUL_ASSIGN", "DIV_ASSIGN", "MOD_ASSIGN", "ADD_ASSIGN", "SUB_ASSIGN",
  "LEFT_ASSIGN", "RIGHT_ASSIGN", "AND_ASSIGN", "XOR_ASSIGN", "OR_ASSIGN",
  "TYPEDEF_NAME", "TYPEDEF", "INLINE", "EXTERN", "STATIC", "AUTO",
  "REGISTER", "THREAD_LOCAL", "BOOL", "CHAR", "SHORT", "INT", "LONG",
  "SIGNED", "UNSIGNED", "FLOAT", "DOUBLE", "CONST", "VOLATILE", "VOID",
  "INT8", "INT16", "INT32", "INT64", "UINT8", "UINT16", "UINT32", "UINT64",
  "SIZE_T", "INTPTR_T", "UINTPTR_T", "STRUCT", "UNION", "ENUM", "ELLIPSIS",
  "CASE", "DEFAULT", "IF", "ELSE", "SWITCH", "WHILE", "DO", "FOR", "GOTO",
  "CONTINUE", "BREAK", "RETURN", "$accept", "translation_unit",
  "external_declaration", "typedef_declaration", "declaration",
  "declaration_specifiers", "type_qualifier", "base_type_specifier",
  "type_specifier", "init_declarator_list", "init_declarator",
  "declarator", "pointer", "type_qualifier_list", "direct_declarator",
  "parameter_type_list", "parameter_list", "parameter_declaration",
  "abstract_declarator", "direct_abstract_declarator",
  "struct_or_union_specifier", "typedef_name_or_identifier",
  "struct_declaration_list", "struct_declaration",
  "specifier_qualifier_list", "struct_declarator_list",
  "struct_declarator", "enum_specifier", "enumerator_list", "enumerator",
  "int_value", YY_NULLPTR
  };
#endif


#if YYDEBUG
  const short
  YYParser::yyrline_[] =
  {
       0,   124,   124,   125,   130,   131,   132,   133,   134,   135,
     139,   143,   144,   148,   149,   150,   151,   155,   156,   160,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     184,   185,   186,   187,   191,   192,   197,   201,   202,   208,
     209,   210,   211,   215,   216,   220,   221,   222,   223,   224,
     225,   229,   230,   234,   235,   239,   240,   241,   245,   246,
     247,   251,   252,   253,   254,   258,   259,   260,   261,   262,
     263,   267,   268,   272,   273,   277,   281,   282,   283,   284,
     288,   289,   293,   300,   301,   302,   303,   304,   308,   309,
     313,   314,   318,   319,   320
  };

  void
  YYParser::yy_stack_print_ () const
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << int (i->state);
    *yycdebug_ << '\n';
  }

  void
  YYParser::yy_reduce_print_ (int yyrule) const
  {
    int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG

  YYParser::symbol_kind_type
  YYParser::yytranslate_ (int t)
  {
    // YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to
    // TOKEN-NUM as returned by yylex.
    static
    const signed char
    translate_table[] =
    {
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87
    };
    // Last valid token kind.
    const int code_max = 342;

    if (t <= 0)
      return symbol_kind::S_YYEOF;
    else if (t <= code_max)
      return YY_CAST (symbol_kind_type, translate_table[t]);
    else
      return symbol_kind::S_YYUNDEF;
  }

#line 13 "c.y"
} // ffi
#line 1923 "yy_parser_generated.cpp"

#line 322 "c.y"


void ffi::YYParser::error(const ffi::Location &loc, const std::string &message) {
  throw ParsingDriver::ParseError{loc, message};
}
