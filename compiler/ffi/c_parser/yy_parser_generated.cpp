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

  case 37: // base_type_specifier: SIZE_T
#line 178 "c.y"
           { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_size); }
#line 813 "yy_parser_generated.cpp"
    break;

  case 38: // type_specifier: base_type_specifier
#line 183 "c.y"
                        { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 819 "yy_parser_generated.cpp"
    break;

  case 39: // type_specifier: struct_or_union_specifier
#line 184 "c.y"
                              { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 825 "yy_parser_generated.cpp"
    break;

  case 40: // type_specifier: enum_specifier
#line 185 "c.y"
                   { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 831 "yy_parser_generated.cpp"
    break;

  case 41: // type_specifier: TYPEDEF_NAME
#line 186 "c.y"
                 { (yylhs.value. type_ ) = driver.get_aliased_type((yystack_[0].value. str_ )); }
#line 837 "yy_parser_generated.cpp"
    break;

  case 42: // init_declarator_list: init_declarator
#line 190 "c.y"
                    { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_initDeclaratorList); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 843 "yy_parser_generated.cpp"
    break;

  case 43: // init_declarator_list: init_declarator_list "," init_declarator
#line 191 "c.y"
                                               { (yylhs.value. type_ ) = (yystack_[2].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 849 "yy_parser_generated.cpp"
    break;

  case 44: // init_declarator: declarator
#line 196 "c.y"
               { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 855 "yy_parser_generated.cpp"
    break;

  case 45: // declarator: direct_declarator
#line 200 "c.y"
                      { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 861 "yy_parser_generated.cpp"
    break;

  case 46: // declarator: pointer direct_declarator
#line 201 "c.y"
                              { (yylhs.value. type_ ) = driver.combine(Pointer{(yystack_[1].value. type_ )}, DirectDeclarator{(yystack_[0].value. type_ )}); }
#line 867 "yy_parser_generated.cpp"
    break;

  case 47: // pointer: "*" type_qualifier_list pointer
#line 207 "c.y"
                                    { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); (yylhs.value. type_ )->num++; }
#line 873 "yy_parser_generated.cpp"
    break;

  case 48: // pointer: "*" type_qualifier_list
#line 208 "c.y"
                            { (yylhs.value. type_ ) = driver.make_pointer(); }
#line 879 "yy_parser_generated.cpp"
    break;

  case 49: // pointer: "*" pointer
#line 209 "c.y"
                { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); (yylhs.value. type_ )->num++; }
#line 885 "yy_parser_generated.cpp"
    break;

  case 50: // pointer: "*"
#line 210 "c.y"
        { (yylhs.value. type_ ) = driver.make_pointer(); }
#line 891 "yy_parser_generated.cpp"
    break;

  case 51: // type_qualifier_list: type_qualifier
#line 214 "c.y"
                   { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typesList); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 897 "yy_parser_generated.cpp"
    break;

  case 52: // type_qualifier_list: type_qualifier_list type_qualifier
#line 215 "c.y"
                                       { (yylhs.value. type_ ) = (yystack_[1].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 903 "yy_parser_generated.cpp"
    break;

  case 53: // direct_declarator: IDENTIFIER
#line 219 "c.y"
               { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Name); (yylhs.value. type_ )->str = (yystack_[0].value. str_ ).to_string(); }
#line 909 "yy_parser_generated.cpp"
    break;

  case 54: // direct_declarator: "(" declarator ")"
#line 220 "c.y"
                             { (yylhs.value. type_ ) = (yystack_[1].value. type_ ); }
#line 915 "yy_parser_generated.cpp"
    break;

  case 55: // direct_declarator: direct_declarator "[" "]"
#line 221 "c.y"
                                        { (yylhs.value. type_ ) = driver.make_array_declarator((yystack_[2].value. type_ ), "-1"); }
#line 921 "yy_parser_generated.cpp"
    break;

  case 56: // direct_declarator: direct_declarator "[" INT_CONSTANT "]"
#line 222 "c.y"
                                                     { (yylhs.value. type_ ) = driver.make_array_declarator((yystack_[3].value. type_ ), (yystack_[1].value. str_ )); }
#line 927 "yy_parser_generated.cpp"
    break;

  case 57: // direct_declarator: direct_declarator "(" parameter_type_list ")"
#line 223 "c.y"
                                                        { (yylhs.value. type_ ) = driver.make_function((yystack_[3].value. type_ ), (yystack_[1].value. type_ )); }
#line 933 "yy_parser_generated.cpp"
    break;

  case 58: // direct_declarator: direct_declarator "(" ")"
#line 224 "c.y"
                                    { (yylhs.value. type_ ) = driver.make_function((yystack_[2].value. type_ ), nullptr); }
#line 939 "yy_parser_generated.cpp"
    break;

  case 59: // parameter_type_list: parameter_list "," ELLIPSIS
#line 228 "c.y"
                                  { (yylhs.value. type_ ) = (yystack_[2].value. type_ ); (yylhs.value. type_ )->members.emplace_back(driver.make_simple_type(FFITypeKind::_ellipsis)); }
#line 945 "yy_parser_generated.cpp"
    break;

  case 60: // parameter_type_list: parameter_list
#line 229 "c.y"
                   { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 951 "yy_parser_generated.cpp"
    break;

  case 61: // parameter_list: parameter_declaration
#line 233 "c.y"
                          { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typesList); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 957 "yy_parser_generated.cpp"
    break;

  case 62: // parameter_list: parameter_list "," parameter_declaration
#line 234 "c.y"
                                               { (yylhs.value. type_ ) = (yystack_[2].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 963 "yy_parser_generated.cpp"
    break;

  case 63: // parameter_declaration: declaration_specifiers declarator
#line 238 "c.y"
                                      { (yylhs.value. type_ ) = driver.combine(DeclarationSpecifiers{(yystack_[1].value. type_ )}, Declarator{(yystack_[0].value. type_ )}); }
#line 969 "yy_parser_generated.cpp"
    break;

  case 64: // parameter_declaration: declaration_specifiers abstract_declarator
#line 239 "c.y"
                                               { (yylhs.value. type_ ) = driver.combine(DeclarationSpecifiers{(yystack_[1].value. type_ )}, AbstractDeclarator{(yystack_[0].value. type_ )}); }
#line 975 "yy_parser_generated.cpp"
    break;

  case 65: // parameter_declaration: declaration_specifiers
#line 240 "c.y"
                           { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 981 "yy_parser_generated.cpp"
    break;

  case 66: // abstract_declarator: pointer direct_abstract_declarator
#line 244 "c.y"
                                       { (yylhs.value. type_ ) = driver.combine(Pointer{(yystack_[1].value. type_ )}, DirectAbstractDeclarator{(yystack_[0].value. type_ )}); }
#line 987 "yy_parser_generated.cpp"
    break;

  case 67: // abstract_declarator: pointer
#line 245 "c.y"
            { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 993 "yy_parser_generated.cpp"
    break;

  case 68: // abstract_declarator: direct_abstract_declarator
#line 246 "c.y"
                               { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 999 "yy_parser_generated.cpp"
    break;

  case 69: // direct_abstract_declarator: "[" "]"
#line 250 "c.y"
                      { (yylhs.value. type_ ) = driver.make_abstract_array_declarator("-1"); }
#line 1005 "yy_parser_generated.cpp"
    break;

  case 70: // direct_abstract_declarator: "[" INT_CONSTANT "]"
#line 251 "c.y"
                                   { (yylhs.value. type_ ) = driver.make_abstract_array_declarator((yystack_[1].value. str_ )); }
#line 1011 "yy_parser_generated.cpp"
    break;

  case 71: // direct_abstract_declarator: direct_abstract_declarator "[" "]"
#line 252 "c.y"
                                                 { (yylhs.value. type_ ) = driver.make_array_declarator((yystack_[2].value. type_ ), "-1"); }
#line 1017 "yy_parser_generated.cpp"
    break;

  case 72: // direct_abstract_declarator: direct_abstract_declarator "[" INT_CONSTANT "]"
#line 253 "c.y"
                                                              { (yylhs.value. type_ ) = driver.make_array_declarator((yystack_[3].value. type_ ), (yystack_[1].value. str_ )); }
#line 1023 "yy_parser_generated.cpp"
    break;

  case 73: // struct_or_union_specifier: STRUCT "{" struct_declaration_list "}"
#line 257 "c.y"
                                                 { (yylhs.value. type_ ) = driver.make_struct_def((yystack_[1].value. type_ )); }
#line 1029 "yy_parser_generated.cpp"
    break;

  case 74: // struct_or_union_specifier: STRUCT typedef_name_or_identifier "{" struct_declaration_list "}"
#line 258 "c.y"
                                                                            { (yylhs.value. type_ ) = driver.make_struct_def((yystack_[1].value. type_ )); (yylhs.value. type_ )->str = (yystack_[3].value. str_ ).to_string(); }
#line 1035 "yy_parser_generated.cpp"
    break;

  case 75: // struct_or_union_specifier: STRUCT typedef_name_or_identifier
#line 259 "c.y"
                                      { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Struct); (yylhs.value. type_ )->str = (yystack_[0].value. str_ ).to_string(); }
#line 1041 "yy_parser_generated.cpp"
    break;

  case 76: // struct_or_union_specifier: UNION "{" struct_declaration_list "}"
#line 260 "c.y"
                                                { (yylhs.value. type_ ) = driver.make_union_def((yystack_[1].value. type_ )); }
#line 1047 "yy_parser_generated.cpp"
    break;

  case 77: // struct_or_union_specifier: UNION typedef_name_or_identifier "{" struct_declaration_list "}"
#line 261 "c.y"
                                                                           { (yylhs.value. type_ ) = driver.make_union_def((yystack_[1].value. type_ )); (yylhs.value. type_ )->str = (yystack_[3].value. str_ ).to_string(); }
#line 1053 "yy_parser_generated.cpp"
    break;

  case 78: // struct_or_union_specifier: UNION typedef_name_or_identifier
#line 262 "c.y"
                                     { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Union); (yylhs.value. type_ )->str = (yystack_[0].value. str_ ).to_string(); }
#line 1059 "yy_parser_generated.cpp"
    break;

  case 79: // typedef_name_or_identifier: IDENTIFIER
#line 266 "c.y"
               { (yylhs.value. str_ ) = (yystack_[0].value. str_ ); }
#line 1065 "yy_parser_generated.cpp"
    break;

  case 80: // typedef_name_or_identifier: TYPEDEF_NAME
#line 267 "c.y"
                 { (yylhs.value. str_ ) = (yystack_[0].value. str_ ); }
#line 1071 "yy_parser_generated.cpp"
    break;

  case 81: // struct_declaration_list: struct_declaration
#line 271 "c.y"
                       { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typesList); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 1077 "yy_parser_generated.cpp"
    break;

  case 82: // struct_declaration_list: struct_declaration_list struct_declaration
#line 272 "c.y"
                                               { (yylhs.value. type_ ) = (yystack_[1].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 1083 "yy_parser_generated.cpp"
    break;

  case 83: // struct_declaration: specifier_qualifier_list struct_declarator_list ";"
#line 276 "c.y"
                                                              { (yylhs.value. type_ ) = (yystack_[1].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[2].value. type_ )); }
#line 1089 "yy_parser_generated.cpp"
    break;

  case 84: // specifier_qualifier_list: type_specifier specifier_qualifier_list
#line 280 "c.y"
                                            { (yylhs.value. type_ ) = driver.combine(TypeSpecifier{(yystack_[1].value. type_ )}, SpecifierQualifierList{(yystack_[0].value. type_ )}); }
#line 1095 "yy_parser_generated.cpp"
    break;

  case 85: // specifier_qualifier_list: type_specifier
#line 281 "c.y"
                   { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 1101 "yy_parser_generated.cpp"
    break;

  case 86: // specifier_qualifier_list: type_qualifier specifier_qualifier_list
#line 282 "c.y"
                                            { (yylhs.value. type_ ) = driver.combine(TypeQualifier{(yystack_[1].value. type_ )}, SpecifierQualifierList{(yystack_[0].value. type_ )}); }
#line 1107 "yy_parser_generated.cpp"
    break;

  case 87: // specifier_qualifier_list: type_qualifier
#line 283 "c.y"
                   { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 1113 "yy_parser_generated.cpp"
    break;

  case 88: // struct_declarator_list: struct_declarator
#line 287 "c.y"
                      { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_structFieldList); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 1119 "yy_parser_generated.cpp"
    break;

  case 89: // struct_declarator_list: struct_declarator_list "," struct_declarator
#line 288 "c.y"
                                                   { (yylhs.value. type_ ) = (yystack_[2].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 1125 "yy_parser_generated.cpp"
    break;

  case 90: // struct_declarator: declarator
#line 292 "c.y"
               { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 1131 "yy_parser_generated.cpp"
    break;

  case 91: // enum_specifier: ENUM "{" enumerator_list "}"
#line 299 "c.y"
                                       { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::EnumDef); }
#line 1137 "yy_parser_generated.cpp"
    break;

  case 92: // enum_specifier: ENUM "{" enumerator_list "," "}"
#line 300 "c.y"
                                             { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::EnumDef); }
#line 1143 "yy_parser_generated.cpp"
    break;

  case 93: // enum_specifier: ENUM IDENTIFIER "{" enumerator_list "}"
#line 301 "c.y"
                                                  { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::EnumDef); (yylhs.value. type_ )->str = (yystack_[3].value. str_ ).to_string(); }
#line 1149 "yy_parser_generated.cpp"
    break;

  case 94: // enum_specifier: ENUM IDENTIFIER "{" enumerator_list "," "}"
#line 302 "c.y"
                                                        { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::EnumDef); (yylhs.value. type_ )->str = (yystack_[4].value. str_ ).to_string(); }
#line 1155 "yy_parser_generated.cpp"
    break;

  case 95: // enum_specifier: ENUM IDENTIFIER
#line 303 "c.y"
                    { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Enum); (yylhs.value. type_ )->str = (yystack_[0].value. str_ ).to_string(); }
#line 1161 "yy_parser_generated.cpp"
    break;

  case 96: // enumerator_list: enumerator
#line 307 "c.y"
               { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_enumList); driver.add_enumerator((yylhs.value. type_ ), (yystack_[0].value. type_ )); }
#line 1167 "yy_parser_generated.cpp"
    break;

  case 97: // enumerator_list: enumerator_list "," enumerator
#line 308 "c.y"
                                     { (yylhs.value. type_ ) = (yystack_[2].value. type_ ); driver.add_enumerator((yylhs.value. type_ ), (yystack_[0].value. type_ )); }
#line 1173 "yy_parser_generated.cpp"
    break;

  case 98: // enumerator: IDENTIFIER "=" int_value
#line 312 "c.y"
                               { (yylhs.value. type_ ) = driver.make_enum_member((yystack_[2].value. str_ ), (yystack_[0].value. int_ )); }
#line 1179 "yy_parser_generated.cpp"
    break;

  case 99: // enumerator: IDENTIFIER
#line 313 "c.y"
               { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_enumMember); (yylhs.value. type_ )->str = (yystack_[0].value. str_ ).to_string(); }
#line 1185 "yy_parser_generated.cpp"
    break;

  case 100: // int_value: INT_CONSTANT
#line 317 "c.y"
                 { (yylhs.value. int_ ) = driver.stoi((yystack_[0].value. str_ )); }
#line 1191 "yy_parser_generated.cpp"
    break;

  case 101: // int_value: "-" INT_CONSTANT
#line 318 "c.y"
                       { (yylhs.value. int_ ) = -driver.stoi((yystack_[0].value. str_ )); }
#line 1197 "yy_parser_generated.cpp"
    break;

  case 102: // int_value: "+" INT_CONSTANT
#line 319 "c.y"
                      { (yylhs.value. int_ ) = driver.stoi((yystack_[0].value. str_ )); }
#line 1203 "yy_parser_generated.cpp"
    break;


#line 1207 "yy_parser_generated.cpp"

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


  const signed char YYParser::yypact_ninf_ = -83;

  const signed char YYParser::yytable_ninf_ = -1;

  const short
  YYParser::yypact_[] =
  {
     326,   -83,   358,   358,   358,   358,   358,   -83,   -83,   -83,
     -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,
     -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,    11,    15,
      44,    93,   -83,   -83,   -83,    17,   358,   -83,   358,   -83,
     -83,   358,   358,    67,   -83,   -83,   -83,   -83,   358,   -83,
     -83,    90,   358,    91,     7,    98,   -83,   -83,   -83,    67,
      -4,   -83,    63,   -83,   -83,    27,    41,   -83,   -83,   -83,
     -83,    79,   125,   -83,    67,   358,   161,   358,    92,    73,
     -83,     7,   100,   -83,   -83,    -4,    67,   -83,    41,   261,
      13,   -83,   -83,   -83,   -83,    84,   -83,   193,   -83,   229,
      78,    55,   -83,    99,   -83,   -83,   -83,   -83,   -83,    62,
     101,   103,   -83,   -83,    97,    67,   -83,   -83,   -83,    94,
     102,   -83,   -83,   -83,   -83,    57,   -83,    70,   -83,    38,
     -83,   104,   -83,   293,   -83,   -83,   -83,   -83,   -83,   -83,
     105,   104,    74,   -83,   -83,   -83,   -83,   106,   -83
  };

  const signed char
  YYParser::yydefact_[] =
  {
       0,    41,     0,     0,     0,     0,     0,    21,    20,    26,
      24,    25,    27,    28,    22,    23,    17,    18,    19,    29,
      30,    31,    32,    33,    34,    35,    36,    37,     0,     0,
       0,     0,     2,     5,     4,     0,    15,    38,    13,    39,
      40,    87,    85,     0,     6,     7,     8,     9,     0,    79,
      80,    75,     0,    78,     0,    95,     1,     3,    11,     0,
      50,    53,     0,    42,    44,     0,    45,    16,    14,    86,
      84,     0,     0,    81,     0,     0,     0,     0,    99,     0,
      96,     0,     0,    51,    49,    48,     0,    12,    46,     0,
       0,    10,    73,    82,    90,     0,    88,     0,    76,     0,
       0,     0,    91,     0,    54,    52,    47,    43,    58,    65,
       0,    60,    61,    55,     0,     0,    83,    74,    77,     0,
       0,   100,    98,    92,    97,     0,    93,     0,    63,    67,
      64,    68,    57,     0,    56,    89,   102,   101,    94,    69,
       0,    66,     0,    59,    62,    70,    71,     0,    72
  };

  const signed char
  YYParser::yypgoto_[] =
  {
     -83,   -83,    80,   -83,    95,   -21,     0,   -83,     8,   -83,
      28,   -34,   -58,   -83,   -64,   -83,   -83,   -16,   -83,   -10,
     -83,    96,   -42,   -35,    16,   -83,     5,   -83,    40,   -82,
     -83
  };

  const unsigned char
  YYParser::yydefgoto_[] =
  {
       0,    31,    32,    33,    34,    35,    41,    37,    42,    62,
      63,    64,    65,    85,    66,   110,   111,   112,   130,   131,
      39,    51,    72,    73,    74,    95,    96,    40,    79,    80,
     122
  };

  const unsigned char
  YYParser::yytable_[] =
  {
      36,    88,    84,    36,    36,    36,    36,    60,    38,    71,
      76,    38,    38,    38,    38,    67,    48,    68,    43,   124,
      52,    58,    78,   113,    59,    82,    49,   106,    60,   114,
      49,    36,    61,    97,    59,    99,    36,    93,    36,    38,
      94,    93,    61,   124,    38,    59,    38,   127,    89,    54,
      90,   129,    50,    61,    16,    17,    50,    69,    70,    55,
      83,   123,    93,   138,    93,    88,    86,    87,   109,    59,
      78,   127,    78,    60,    59,   128,   101,    61,    60,   102,
     139,    94,    61,    91,   146,   105,   140,   115,   116,    36,
     147,   119,   120,    56,   121,    75,    77,    38,    44,    45,
      46,    47,   125,    81,   100,   126,   133,   134,   104,   132,
     136,    57,   109,   142,   107,   145,   148,   144,   137,   141,
     135,   103,     0,     0,     0,    53,     0,     0,     0,     0,
       0,    92,     0,    36,     1,     2,     0,     3,     4,     5,
       6,    38,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,     1,    98,     0,     0,
       0,     0,     0,     0,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,     0,   117,
       0,     0,     1,     0,     0,     0,     0,     0,     0,     0,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,     1,   118,     0,     0,     0,     0,
       0,     0,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,     0,     0,     0,   108,
       1,     0,     0,     0,     0,     0,     0,     0,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,     1,     0,     0,     0,     0,     0,     0,     0,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,     1,     0,     0,     0,     0,     0,
       0,     0,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,   143,     1,     2,     0,
       3,     4,     5,     6,     0,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,     1,
       0,     0,     0,     0,     0,     0,     0,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30
  };

  const short
  YYParser::yycheck_[] =
  {
       0,    65,    60,     3,     4,     5,     6,    11,     0,    43,
      52,     3,     4,     5,     6,    36,     5,    38,     2,   101,
       5,     4,    15,    10,     7,    59,    15,    85,    11,    16,
      15,    31,    15,    75,     7,    77,    36,    72,    38,    31,
      74,    76,    15,   125,    36,     7,    38,     9,     7,     5,
       9,   109,    41,    15,    58,    59,    41,    41,    42,    15,
      60,     6,    97,     6,    99,   129,     3,     4,    89,     7,
      15,     9,    15,    11,     7,   109,     3,    15,    11,     6,
      10,   115,    15,     4,    10,    85,    16,     3,     4,    89,
      16,    13,    14,     0,    16,     5,     5,    89,     3,     4,
       5,     6,     3,     5,    12,     6,     3,    10,     8,     8,
      16,    31,   133,     9,    86,    10,    10,   133,    16,   129,
     115,    81,    -1,    -1,    -1,    29,    -1,    -1,    -1,    -1,
      -1,     6,    -1,   133,    41,    42,    -1,    44,    45,    46,
      47,   133,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    41,     6,    -1,    -1,
      -1,    -1,    -1,    -1,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    -1,     6,
      -1,    -1,    41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    41,     6,    -1,    -1,    -1,    -1,
      -1,    -1,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    -1,    -1,    -1,     8,
      41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    41,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    41,    42,    -1,
      44,    45,    46,    47,    -1,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    41,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72
  };

  const signed char
  YYParser::yystos_[] =
  {
       0,    41,    42,    44,    45,    46,    47,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    87,    88,    89,    90,    91,    92,    93,    94,   106,
     113,    92,    94,   110,    90,    90,    90,    90,     5,    15,
      41,   107,     5,   107,     5,    15,     0,    88,     4,     7,
      11,    15,    95,    96,    97,    98,   100,    91,    91,   110,
     110,    97,   108,   109,   110,     5,   108,     5,    15,   114,
     115,     5,    97,    92,    98,    99,     3,     4,   100,     7,
       9,     4,     6,   109,    97,   111,   112,   108,     6,   108,
      12,     3,     6,   114,     8,    92,    98,    96,     8,    91,
     101,   102,   103,    10,    16,     3,     4,     6,     6,    13,
      14,    16,   116,     6,   115,     3,     6,     9,    97,    98,
     104,   105,     8,     3,    10,   112,    16,    16,     6,    10,
      16,   105,     9,    73,   103,    10,    10,    16,    10
  };

  const signed char
  YYParser::yyr1_[] =
  {
       0,    86,    87,    87,    88,    88,    88,    88,    88,    88,
      89,    90,    90,    91,    91,    91,    91,    92,    92,    93,
      93,    93,    93,    93,    93,    93,    93,    93,    93,    93,
      93,    93,    93,    93,    93,    93,    93,    93,    94,    94,
      94,    94,    95,    95,    96,    97,    97,    98,    98,    98,
      98,    99,    99,   100,   100,   100,   100,   100,   100,   101,
     101,   102,   102,   103,   103,   103,   104,   104,   104,   105,
     105,   105,   105,   106,   106,   106,   106,   106,   106,   107,
     107,   108,   108,   109,   110,   110,   110,   110,   111,   111,
     112,   113,   113,   113,   113,   113,   114,   114,   115,   115,
     116,   116,   116
  };

  const signed char
  YYParser::yyr2_[] =
  {
       0,     2,     1,     2,     1,     1,     2,     2,     2,     2,
       4,     2,     3,     1,     2,     1,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     1,     1,     2,     3,     2,     2,
       1,     1,     2,     1,     3,     3,     4,     4,     3,     3,
       1,     1,     3,     2,     2,     1,     2,     1,     1,     2,
       3,     3,     4,     4,     5,     2,     4,     5,     2,     1,
       1,     1,     2,     3,     2,     1,     2,     1,     1,     3,
       1,     4,     5,     5,     6,     2,     1,     3,     3,     1,
       1,     2,     2
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
  "SIZE_T", "STRUCT", "UNION", "ENUM", "ELLIPSIS", "CASE", "DEFAULT", "IF",
  "ELSE", "SWITCH", "WHILE", "DO", "FOR", "GOTO", "CONTINUE", "BREAK",
  "RETURN", "$accept", "translation_unit", "external_declaration",
  "typedef_declaration", "declaration", "declaration_specifiers",
  "type_qualifier", "base_type_specifier", "type_specifier",
  "init_declarator_list", "init_declarator", "declarator", "pointer",
  "type_qualifier_list", "direct_declarator", "parameter_type_list",
  "parameter_list", "parameter_declaration", "abstract_declarator",
  "direct_abstract_declarator", "struct_or_union_specifier",
  "typedef_name_or_identifier", "struct_declaration_list",
  "struct_declaration", "specifier_qualifier_list",
  "struct_declarator_list", "struct_declarator", "enum_specifier",
  "enumerator_list", "enumerator", "int_value", YY_NULLPTR
  };
#endif


#if YYDEBUG
  const short
  YYParser::yyrline_[] =
  {
       0,   124,   124,   125,   130,   131,   132,   133,   134,   135,
     139,   143,   144,   148,   149,   150,   151,   155,   156,   160,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   183,   184,
     185,   186,   190,   191,   196,   200,   201,   207,   208,   209,
     210,   214,   215,   219,   220,   221,   222,   223,   224,   228,
     229,   233,   234,   238,   239,   240,   244,   245,   246,   250,
     251,   252,   253,   257,   258,   259,   260,   261,   262,   266,
     267,   271,   272,   276,   280,   281,   282,   283,   287,   288,
     292,   299,   300,   301,   302,   303,   307,   308,   312,   313,
     317,   318,   319
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
      85
    };
    // Last valid token kind.
    const int code_max = 340;

    if (t <= 0)
      return symbol_kind::S_YYEOF;
    else if (t <= code_max)
      return YY_CAST (symbol_kind_type, translate_table[t]);
    else
      return symbol_kind::S_YYUNDEF;
  }

#line 13 "c.y"
} // ffi
#line 1905 "yy_parser_generated.cpp"

#line 321 "c.y"


void ffi::YYParser::error(const ffi::Location &loc, const std::string &message) {
  throw ParsingDriver::ParseError{loc, message};
}
