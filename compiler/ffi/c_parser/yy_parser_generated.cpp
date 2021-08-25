// A Bison parser, made by GNU Bison 3.7.

// Skeleton implementation for Bison LALR(1) parsers in C++

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
# define YY_SYMBOL_PRINT(Title, Symbol)  YYUSE (Symbol)
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
  YYParser::by_kind::clear ()
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
    YYUSE (yysym.kind ());
  }

#if YYDEBUG
  template <typename Base>
  void
  YYParser::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YYUSE (yyoutput);
    if (yysym.empty ())
      yyo << "empty symbol";
    else
      {
        symbol_kind_type yykind = yysym.kind ();
        yyo << (yykind < YYNTOKENS ? "token" : "nterm")
            << ' ' << yysym.name () << " ("
            << yysym.location << ": ";
        YYUSE (yykind);
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
#line 129 "c.y"
                { driver.add_type((yystack_[0].value. type_ )); }
#line 615 "yy_parser_generated.cpp"
    break;

  case 5: // external_declaration: typedef_declaration
#line 130 "c.y"
                        {}
#line 621 "yy_parser_generated.cpp"
    break;

  case 6: // external_declaration: EXTERN declaration
#line 131 "c.y"
                       { driver.add_type((yystack_[0].value. type_ )); }
#line 627 "yy_parser_generated.cpp"
    break;

  case 7: // external_declaration: STATIC declaration
#line 132 "c.y"
                       { driver.add_type((yystack_[0].value. type_ )); }
#line 633 "yy_parser_generated.cpp"
    break;

  case 8: // external_declaration: AUTO declaration
#line 133 "c.y"
                     { driver.add_type((yystack_[0].value. type_ )); }
#line 639 "yy_parser_generated.cpp"
    break;

  case 9: // external_declaration: REGISTER declaration
#line 134 "c.y"
                         { driver.add_type((yystack_[0].value. type_ )); }
#line 645 "yy_parser_generated.cpp"
    break;

  case 10: // typedef_declaration: TYPEDEF specifier_qualifier_list declarator ";"
#line 138 "c.y"
                                                          { driver.add_typedef((yystack_[2].value. type_ ), (yystack_[1].value. type_ )); }
#line 651 "yy_parser_generated.cpp"
    break;

  case 11: // declaration: declaration_specifiers ";"
#line 142 "c.y"
                                     { (yylhs.value. type_ ) = (yystack_[1].value. type_ ); }
#line 657 "yy_parser_generated.cpp"
    break;

  case 12: // declaration: declaration_specifiers init_declarator_list ";"
#line 143 "c.y"
                                                          { (yylhs.value. type_ ) = driver.combine(DeclarationSpecifiers{(yystack_[2].value. type_ )}, InitDeclaratorList{(yystack_[1].value. type_ )}); }
#line 663 "yy_parser_generated.cpp"
    break;

  case 13: // declaration_specifiers: type_specifier
#line 147 "c.y"
                   { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 669 "yy_parser_generated.cpp"
    break;

  case 14: // declaration_specifiers: type_specifier declaration_specifiers
#line 148 "c.y"
                                          { (yylhs.value. type_ ) = driver.combine(TypeSpecifier{(yystack_[1].value. type_ )}, DeclarationSpecifiers{(yystack_[0].value. type_ )}); }
#line 675 "yy_parser_generated.cpp"
    break;

  case 15: // declaration_specifiers: type_qualifier
#line 149 "c.y"
                   { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 681 "yy_parser_generated.cpp"
    break;

  case 16: // declaration_specifiers: type_qualifier declaration_specifiers
#line 150 "c.y"
                                          { (yylhs.value. type_ ) = driver.combine(TypeQualifier{(yystack_[1].value. type_ )}, DeclarationSpecifiers{(yystack_[0].value. type_ )}); }
#line 687 "yy_parser_generated.cpp"
    break;

  case 17: // type_qualifier: CONST
#line 154 "c.y"
          { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Const); }
#line 693 "yy_parser_generated.cpp"
    break;

  case 18: // type_qualifier: VOLATILE
#line 155 "c.y"
             { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Volatile); }
#line 699 "yy_parser_generated.cpp"
    break;

  case 19: // base_type_specifier: VOID
#line 159 "c.y"
         { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Void); }
#line 705 "yy_parser_generated.cpp"
    break;

  case 20: // base_type_specifier: CHAR
#line 160 "c.y"
         { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Char); }
#line 711 "yy_parser_generated.cpp"
    break;

  case 21: // base_type_specifier: BOOL
#line 161 "c.y"
         { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Bool); }
#line 717 "yy_parser_generated.cpp"
    break;

  case 22: // base_type_specifier: FLOAT
#line 162 "c.y"
          { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Float); }
#line 723 "yy_parser_generated.cpp"
    break;

  case 23: // base_type_specifier: DOUBLE
#line 163 "c.y"
           { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Double); }
#line 729 "yy_parser_generated.cpp"
    break;

  case 24: // base_type_specifier: INT
#line 164 "c.y"
        { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_int); }
#line 735 "yy_parser_generated.cpp"
    break;

  case 25: // base_type_specifier: LONG
#line 165 "c.y"
         { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Long); }
#line 741 "yy_parser_generated.cpp"
    break;

  case 26: // base_type_specifier: SHORT
#line 166 "c.y"
          { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Short); }
#line 747 "yy_parser_generated.cpp"
    break;

  case 27: // base_type_specifier: SIGNED
#line 167 "c.y"
           { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Signed); }
#line 753 "yy_parser_generated.cpp"
    break;

  case 28: // base_type_specifier: UNSIGNED
#line 168 "c.y"
             { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Unsigned); }
#line 759 "yy_parser_generated.cpp"
    break;

  case 29: // base_type_specifier: INT8
#line 169 "c.y"
         { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Int8); }
#line 765 "yy_parser_generated.cpp"
    break;

  case 30: // base_type_specifier: INT16
#line 170 "c.y"
          { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Int16); }
#line 771 "yy_parser_generated.cpp"
    break;

  case 31: // base_type_specifier: INT32
#line 171 "c.y"
          { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Int32); }
#line 777 "yy_parser_generated.cpp"
    break;

  case 32: // base_type_specifier: INT64
#line 172 "c.y"
          { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Int64); }
#line 783 "yy_parser_generated.cpp"
    break;

  case 33: // base_type_specifier: UINT8
#line 173 "c.y"
          { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Uint8); }
#line 789 "yy_parser_generated.cpp"
    break;

  case 34: // base_type_specifier: UINT16
#line 174 "c.y"
           { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Uint16); }
#line 795 "yy_parser_generated.cpp"
    break;

  case 35: // base_type_specifier: UINT32
#line 175 "c.y"
           { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Uint32); }
#line 801 "yy_parser_generated.cpp"
    break;

  case 36: // base_type_specifier: UINT64
#line 176 "c.y"
           { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Uint64); }
#line 807 "yy_parser_generated.cpp"
    break;

  case 37: // base_type_specifier: SIZE_T
#line 177 "c.y"
           { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_size); }
#line 813 "yy_parser_generated.cpp"
    break;

  case 38: // type_specifier: base_type_specifier
#line 182 "c.y"
                        { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 819 "yy_parser_generated.cpp"
    break;

  case 39: // type_specifier: struct_or_union_specifier
#line 183 "c.y"
                              { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 825 "yy_parser_generated.cpp"
    break;

  case 40: // type_specifier: enum_specifier
#line 184 "c.y"
                   { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 831 "yy_parser_generated.cpp"
    break;

  case 41: // type_specifier: TYPEDEF_NAME
#line 185 "c.y"
                 { (yylhs.value. type_ ) = driver.get_aliased_type((yystack_[0].value. str_ )); }
#line 837 "yy_parser_generated.cpp"
    break;

  case 42: // init_declarator_list: init_declarator
#line 189 "c.y"
                    { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_initDeclaratorList); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 843 "yy_parser_generated.cpp"
    break;

  case 43: // init_declarator_list: init_declarator_list "," init_declarator
#line 190 "c.y"
                                               { (yylhs.value. type_ ) = (yystack_[2].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 849 "yy_parser_generated.cpp"
    break;

  case 44: // init_declarator: declarator
#line 195 "c.y"
               { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 855 "yy_parser_generated.cpp"
    break;

  case 45: // declarator: direct_declarator
#line 199 "c.y"
                      { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 861 "yy_parser_generated.cpp"
    break;

  case 46: // declarator: pointer direct_declarator
#line 200 "c.y"
                              { (yylhs.value. type_ ) = driver.combine(Pointer{(yystack_[1].value. type_ )}, DirectDeclarator{(yystack_[0].value. type_ )}); }
#line 867 "yy_parser_generated.cpp"
    break;

  case 47: // pointer: "*" type_qualifier_list pointer
#line 206 "c.y"
                                    { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); (yylhs.value. type_ )->num++; }
#line 873 "yy_parser_generated.cpp"
    break;

  case 48: // pointer: "*" type_qualifier_list
#line 207 "c.y"
                            { (yylhs.value. type_ ) = driver.make_pointer(); }
#line 879 "yy_parser_generated.cpp"
    break;

  case 49: // pointer: "*" pointer
#line 208 "c.y"
                { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); (yylhs.value. type_ )->num++; }
#line 885 "yy_parser_generated.cpp"
    break;

  case 50: // pointer: "*"
#line 209 "c.y"
        { (yylhs.value. type_ ) = driver.make_pointer(); }
#line 891 "yy_parser_generated.cpp"
    break;

  case 51: // type_qualifier_list: type_qualifier
#line 213 "c.y"
                   { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typesList); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 897 "yy_parser_generated.cpp"
    break;

  case 52: // type_qualifier_list: type_qualifier_list type_qualifier
#line 214 "c.y"
                                       { (yylhs.value. type_ ) = (yystack_[1].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 903 "yy_parser_generated.cpp"
    break;

  case 53: // direct_declarator: IDENTIFIER
#line 218 "c.y"
               { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Name); (yylhs.value. type_ )->str = (yystack_[0].value. str_ ).to_string(); }
#line 909 "yy_parser_generated.cpp"
    break;

  case 54: // direct_declarator: "(" declarator ")"
#line 219 "c.y"
                             { (yylhs.value. type_ ) = (yystack_[1].value. type_ ); }
#line 915 "yy_parser_generated.cpp"
    break;

  case 55: // direct_declarator: direct_declarator "[" "]"
#line 220 "c.y"
                                        { (yylhs.value. type_ ) = driver.make_pointer(); (yylhs.value. type_ ) = driver.combine(Pointer{(yylhs.value. type_ )}, DirectDeclarator{(yystack_[2].value. type_ )}); }
#line 921 "yy_parser_generated.cpp"
    break;

  case 56: // direct_declarator: direct_declarator "[" INT_CONSTANT "]"
#line 221 "c.y"
                                                     { (yylhs.value. type_ ) = driver.make_array_declarator((yystack_[3].value. type_ ), (yystack_[1].value. str_ )); }
#line 927 "yy_parser_generated.cpp"
    break;

  case 57: // direct_declarator: direct_declarator "(" parameter_type_list ")"
#line 222 "c.y"
                                                        { (yylhs.value. type_ ) = driver.make_function((yystack_[3].value. type_ ), (yystack_[1].value. type_ )); }
#line 933 "yy_parser_generated.cpp"
    break;

  case 58: // direct_declarator: direct_declarator "(" ")"
#line 223 "c.y"
                                    { (yylhs.value. type_ ) = driver.make_function((yystack_[2].value. type_ ), nullptr); }
#line 939 "yy_parser_generated.cpp"
    break;

  case 59: // parameter_type_list: parameter_list "," ELLIPSIS
#line 227 "c.y"
                                  { (yylhs.value. type_ ) = (yystack_[2].value. type_ ); (yylhs.value. type_ )->members.emplace_back(driver.make_simple_type(FFITypeKind::_ellipsis)); }
#line 945 "yy_parser_generated.cpp"
    break;

  case 60: // parameter_type_list: parameter_list
#line 228 "c.y"
                   { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 951 "yy_parser_generated.cpp"
    break;

  case 61: // parameter_list: parameter_declaration
#line 232 "c.y"
                          { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typesList); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 957 "yy_parser_generated.cpp"
    break;

  case 62: // parameter_list: parameter_list "," parameter_declaration
#line 233 "c.y"
                                               { (yylhs.value. type_ ) = (yystack_[2].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 963 "yy_parser_generated.cpp"
    break;

  case 63: // parameter_declaration: declaration_specifiers declarator
#line 237 "c.y"
                                      { (yylhs.value. type_ ) = driver.combine(DeclarationSpecifiers{(yystack_[1].value. type_ )}, Declarator{(yystack_[0].value. type_ )}); }
#line 969 "yy_parser_generated.cpp"
    break;

  case 64: // parameter_declaration: declaration_specifiers abstract_declarator
#line 238 "c.y"
                                               { (yylhs.value. type_ ) = driver.combine(DeclarationSpecifiers{(yystack_[1].value. type_ )}, AbstractDeclarator{(yystack_[0].value. type_ )}); }
#line 975 "yy_parser_generated.cpp"
    break;

  case 65: // parameter_declaration: declaration_specifiers
#line 239 "c.y"
                           { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 981 "yy_parser_generated.cpp"
    break;

  case 66: // abstract_declarator: pointer
#line 243 "c.y"
            { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 987 "yy_parser_generated.cpp"
    break;

  case 67: // struct_or_union_specifier: STRUCT "{" struct_declaration_list "}"
#line 247 "c.y"
                                                 { (yylhs.value. type_ ) = driver.make_struct_def((yystack_[1].value. type_ )); }
#line 993 "yy_parser_generated.cpp"
    break;

  case 68: // struct_or_union_specifier: STRUCT typedef_name_or_identifier "{" struct_declaration_list "}"
#line 248 "c.y"
                                                                            { (yylhs.value. type_ ) = driver.make_struct_def((yystack_[1].value. type_ )); (yylhs.value. type_ )->str = (yystack_[3].value. str_ ).to_string(); }
#line 999 "yy_parser_generated.cpp"
    break;

  case 69: // struct_or_union_specifier: STRUCT typedef_name_or_identifier
#line 249 "c.y"
                                      { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Struct); (yylhs.value. type_ )->str = (yystack_[0].value. str_ ).to_string(); }
#line 1005 "yy_parser_generated.cpp"
    break;

  case 70: // struct_or_union_specifier: UNION "{" struct_declaration_list "}"
#line 250 "c.y"
                                                { (yylhs.value. type_ ) = driver.make_union_def((yystack_[1].value. type_ )); }
#line 1011 "yy_parser_generated.cpp"
    break;

  case 71: // struct_or_union_specifier: UNION typedef_name_or_identifier "{" struct_declaration_list "}"
#line 251 "c.y"
                                                                           { (yylhs.value. type_ ) = driver.make_union_def((yystack_[1].value. type_ )); (yylhs.value. type_ )->str = (yystack_[3].value. str_ ).to_string(); }
#line 1017 "yy_parser_generated.cpp"
    break;

  case 72: // struct_or_union_specifier: UNION typedef_name_or_identifier
#line 252 "c.y"
                                     { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Union); (yylhs.value. type_ )->str = (yystack_[0].value. str_ ).to_string(); }
#line 1023 "yy_parser_generated.cpp"
    break;

  case 73: // typedef_name_or_identifier: IDENTIFIER
#line 256 "c.y"
               { (yylhs.value. str_ ) = (yystack_[0].value. str_ ); }
#line 1029 "yy_parser_generated.cpp"
    break;

  case 74: // typedef_name_or_identifier: TYPEDEF_NAME
#line 257 "c.y"
                 { (yylhs.value. str_ ) = (yystack_[0].value. str_ ); }
#line 1035 "yy_parser_generated.cpp"
    break;

  case 75: // struct_declaration_list: struct_declaration
#line 261 "c.y"
                       { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_typesList); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 1041 "yy_parser_generated.cpp"
    break;

  case 76: // struct_declaration_list: struct_declaration_list struct_declaration
#line 262 "c.y"
                                               { (yylhs.value. type_ ) = (yystack_[1].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 1047 "yy_parser_generated.cpp"
    break;

  case 77: // struct_declaration: specifier_qualifier_list struct_declarator_list ";"
#line 266 "c.y"
                                                              { (yylhs.value. type_ ) = (yystack_[1].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[2].value. type_ )); }
#line 1053 "yy_parser_generated.cpp"
    break;

  case 78: // specifier_qualifier_list: type_specifier specifier_qualifier_list
#line 270 "c.y"
                                            { (yylhs.value. type_ ) = driver.combine(TypeSpecifier{(yystack_[1].value. type_ )}, SpecifierQualifierList{(yystack_[0].value. type_ )}); }
#line 1059 "yy_parser_generated.cpp"
    break;

  case 79: // specifier_qualifier_list: type_specifier
#line 271 "c.y"
                   { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 1065 "yy_parser_generated.cpp"
    break;

  case 80: // specifier_qualifier_list: type_qualifier specifier_qualifier_list
#line 272 "c.y"
                                            { (yylhs.value. type_ ) = driver.combine(TypeQualifier{(yystack_[1].value. type_ )}, SpecifierQualifierList{(yystack_[0].value. type_ )}); }
#line 1071 "yy_parser_generated.cpp"
    break;

  case 81: // specifier_qualifier_list: type_qualifier
#line 273 "c.y"
                   { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 1077 "yy_parser_generated.cpp"
    break;

  case 82: // struct_declarator_list: struct_declarator
#line 277 "c.y"
                      { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_structFieldList); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 1083 "yy_parser_generated.cpp"
    break;

  case 83: // struct_declarator_list: struct_declarator_list "," struct_declarator
#line 278 "c.y"
                                                   { (yylhs.value. type_ ) = (yystack_[2].value. type_ ); (yylhs.value. type_ )->members.emplace_back((yystack_[0].value. type_ )); }
#line 1089 "yy_parser_generated.cpp"
    break;

  case 84: // struct_declarator: declarator
#line 282 "c.y"
               { (yylhs.value. type_ ) = (yystack_[0].value. type_ ); }
#line 1095 "yy_parser_generated.cpp"
    break;

  case 85: // enum_specifier: ENUM "{" enumerator_list "}"
#line 289 "c.y"
                                       { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::EnumDef); }
#line 1101 "yy_parser_generated.cpp"
    break;

  case 86: // enum_specifier: ENUM "{" enumerator_list "," "}"
#line 290 "c.y"
                                             { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::EnumDef); }
#line 1107 "yy_parser_generated.cpp"
    break;

  case 87: // enum_specifier: ENUM IDENTIFIER "{" enumerator_list "}"
#line 291 "c.y"
                                                  { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::EnumDef); (yylhs.value. type_ )->str = (yystack_[3].value. str_ ).to_string(); }
#line 1113 "yy_parser_generated.cpp"
    break;

  case 88: // enum_specifier: ENUM IDENTIFIER "{" enumerator_list "," "}"
#line 292 "c.y"
                                                        { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::EnumDef); (yylhs.value. type_ )->str = (yystack_[4].value. str_ ).to_string(); }
#line 1119 "yy_parser_generated.cpp"
    break;

  case 89: // enum_specifier: ENUM IDENTIFIER
#line 293 "c.y"
                    { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::Enum); (yylhs.value. type_ )->str = (yystack_[0].value. str_ ).to_string(); }
#line 1125 "yy_parser_generated.cpp"
    break;

  case 90: // enumerator_list: enumerator
#line 297 "c.y"
               { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_enumList); driver.add_enumerator((yylhs.value. type_ ), (yystack_[0].value. type_ )); }
#line 1131 "yy_parser_generated.cpp"
    break;

  case 91: // enumerator_list: enumerator_list "," enumerator
#line 298 "c.y"
                                     { (yylhs.value. type_ ) = (yystack_[2].value. type_ ); driver.add_enumerator((yylhs.value. type_ ), (yystack_[0].value. type_ )); }
#line 1137 "yy_parser_generated.cpp"
    break;

  case 92: // enumerator: IDENTIFIER "=" int_value
#line 302 "c.y"
                               { (yylhs.value. type_ ) = driver.make_enum_member((yystack_[2].value. str_ ), (yystack_[0].value. int_ )); }
#line 1143 "yy_parser_generated.cpp"
    break;

  case 93: // enumerator: IDENTIFIER
#line 303 "c.y"
               { (yylhs.value. type_ ) = driver.make_simple_type(FFITypeKind::_enumMember); (yylhs.value. type_ )->str = (yystack_[0].value. str_ ).to_string(); }
#line 1149 "yy_parser_generated.cpp"
    break;

  case 94: // int_value: INT_CONSTANT
#line 307 "c.y"
                 { (yylhs.value. int_ ) = driver.stoi((yystack_[0].value. str_ )); }
#line 1155 "yy_parser_generated.cpp"
    break;

  case 95: // int_value: "-" INT_CONSTANT
#line 308 "c.y"
                       { (yylhs.value. int_ ) = -driver.stoi((yystack_[0].value. str_ )); }
#line 1161 "yy_parser_generated.cpp"
    break;

  case 96: // int_value: "+" INT_CONSTANT
#line 309 "c.y"
                      { (yylhs.value. int_ ) = driver.stoi((yystack_[0].value. str_ )); }
#line 1167 "yy_parser_generated.cpp"
    break;


#line 1171 "yy_parser_generated.cpp"

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
     324,   -83,   356,   356,   356,   356,   356,   -83,   -83,   -83,
     -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,
     -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,    11,    15,
      32,    91,   -83,   -83,   -83,    17,   356,   -83,   356,   -83,
     -83,   356,   356,    52,   -83,   -83,   -83,   -83,   356,   -83,
     -83,    43,   356,    56,     7,    60,   -83,   -83,   -83,    52,
      -4,   -83,    89,   -83,   -83,    27,    81,   -83,   -83,   -83,
     -83,    69,   123,   -83,    52,   356,   159,   356,    75,    63,
     -83,     7,    72,   -83,   -83,    -4,    52,   -83,    81,   259,
      13,   -83,   -83,   -83,   -83,    92,   -83,   191,   -83,   227,
      70,    35,   -83,    68,   -83,   -83,   -83,   -83,   -83,    52,
      74,    95,   -83,   -83,    84,    52,   -83,   -83,   -83,    83,
      85,   -83,   -83,   -83,   -83,    47,   -83,   -83,    27,   -83,
     -83,   291,   -83,   -83,   -83,   -83,   -83,   -83,   -83
  };

  const signed char
  YYParser::yydefact_[] =
  {
       0,    41,     0,     0,     0,     0,     0,    21,    20,    26,
      24,    25,    27,    28,    22,    23,    17,    18,    19,    29,
      30,    31,    32,    33,    34,    35,    36,    37,     0,     0,
       0,     0,     2,     5,     4,     0,    15,    38,    13,    39,
      40,    81,    79,     0,     6,     7,     8,     9,     0,    73,
      74,    69,     0,    72,     0,    89,     1,     3,    11,     0,
      50,    53,     0,    42,    44,     0,    45,    16,    14,    80,
      78,     0,     0,    75,     0,     0,     0,     0,    93,     0,
      90,     0,     0,    51,    49,    48,     0,    12,    46,     0,
       0,    10,    67,    76,    84,     0,    82,     0,    70,     0,
       0,     0,    85,     0,    54,    52,    47,    43,    58,    65,
       0,    60,    61,    55,     0,     0,    77,    68,    71,     0,
       0,    94,    92,    86,    91,     0,    87,    63,    66,    64,
      57,     0,    56,    83,    96,    95,    88,    59,    62
  };

  const signed char
  YYParser::yypgoto_[] =
  {
     -83,   -83,    71,   -83,    73,   -21,     0,   -83,     8,   -83,
      14,   -34,   -58,   -83,   -64,   -83,   -83,   -28,   -83,   -83,
      76,   -42,   -27,    16,   -83,   -11,   -83,    25,   -82,   -83
  };

  const short
  YYParser::yydefgoto_[] =
  {
      -1,    31,    32,    33,    34,    35,    41,    37,    42,    62,
      63,    64,    65,    85,    66,   110,   111,   112,   129,    39,
      51,    72,    73,    74,    95,    96,    40,    79,    80,   122
  };

  const unsigned char
  YYParser::yytable_[] =
  {
      36,    88,    84,    36,    36,    36,    36,    60,    38,    71,
      76,    38,    38,    38,    38,    67,    48,    68,    43,   124,
      52,    58,    78,   113,    59,    82,    49,   106,    60,   114,
      49,    36,    61,    97,    59,    99,    36,    54,    36,    38,
      94,   123,    61,   124,    38,    93,    38,    55,    75,    93,
      78,   128,    50,   136,    16,    17,    50,    69,    70,    59,
      83,    77,    78,    60,    88,    81,   101,    61,   109,   102,
      93,   125,    93,    91,   126,   127,    44,    45,    46,    47,
     104,    94,   130,   119,   120,   105,   121,   100,    89,    36,
      90,    56,    86,    87,   132,   115,   116,    38,   131,   134,
     107,   135,    57,   138,   133,    53,   103,     0,     0,     0,
     109,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    92,
       0,    36,     1,     2,     0,     3,     4,     5,     6,    38,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,     1,    98,     0,     0,     0,     0,
       0,     0,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,     0,   117,     0,     0,
       1,     0,     0,     0,     0,     0,     0,     0,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,     1,   118,     0,     0,     0,     0,     0,     0,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,     0,     0,     0,   108,     1,     0,
       0,     0,     0,     0,     0,     0,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
       1,     0,     0,     0,     0,     0,     0,     0,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,     1,     0,     0,     0,     0,     0,     0,     0,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,   137,     1,     2,     0,     3,     4,
       5,     6,     0,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,     1,     0,     0,
       0,     0,     0,     0,     0,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30
  };

  const short
  YYParser::yycheck_[] =
  {
       0,    65,    60,     3,     4,     5,     6,    11,     0,    43,
      52,     3,     4,     5,     6,    36,     5,    38,     2,   101,
       5,     4,    15,    10,     7,    59,    15,    85,    11,    16,
      15,    31,    15,    75,     7,    77,    36,     5,    38,    31,
      74,     6,    15,   125,    36,    72,    38,    15,     5,    76,
      15,   109,    41,     6,    58,    59,    41,    41,    42,     7,
      60,     5,    15,    11,   128,     5,     3,    15,    89,     6,
      97,     3,    99,     4,     6,   109,     3,     4,     5,     6,
       8,   115,     8,    13,    14,    85,    16,    12,     7,    89,
       9,     0,     3,     4,    10,     3,     4,    89,     3,    16,
      86,    16,    31,   131,   115,    29,    81,    -1,    -1,    -1,
     131,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     6,
      -1,   131,    41,    42,    -1,    44,    45,    46,    47,   131,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    41,     6,    -1,    -1,    -1,    -1,
      -1,    -1,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    -1,     6,    -1,    -1,
      41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    41,     6,    -1,    -1,    -1,    -1,    -1,    -1,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    -1,    -1,    -1,     8,    41,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    41,    42,    -1,    44,    45,
      46,    47,    -1,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    41,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72
  };

  const signed char
  YYParser::yystos_[] =
  {
       0,    41,    42,    44,    45,    46,    47,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    87,    88,    89,    90,    91,    92,    93,    94,   105,
     112,    92,    94,   109,    90,    90,    90,    90,     5,    15,
      41,   106,     5,   106,     5,    15,     0,    88,     4,     7,
      11,    15,    95,    96,    97,    98,   100,    91,    91,   109,
     109,    97,   107,   108,   109,     5,   107,     5,    15,   113,
     114,     5,    97,    92,    98,    99,     3,     4,   100,     7,
       9,     4,     6,   108,    97,   110,   111,   107,     6,   107,
      12,     3,     6,   113,     8,    92,    98,    96,     8,    91,
     101,   102,   103,    10,    16,     3,     4,     6,     6,    13,
      14,    16,   115,     6,   114,     3,     6,    97,    98,   104,
       8,     3,    10,   111,    16,    16,     6,    73,   103
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
     101,   102,   102,   103,   103,   103,   104,   105,   105,   105,
     105,   105,   105,   106,   106,   107,   107,   108,   109,   109,
     109,   109,   110,   110,   111,   112,   112,   112,   112,   112,
     113,   113,   114,   114,   115,   115,   115
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
       1,     1,     3,     2,     2,     1,     1,     4,     5,     2,
       4,     5,     2,     1,     1,     1,     2,     3,     2,     1,
       2,     1,     1,     3,     1,     4,     5,     5,     6,     2,
       1,     3,     3,     1,     1,     2,     2
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
       0,   123,   123,   124,   129,   130,   131,   132,   133,   134,
     138,   142,   143,   147,   148,   149,   150,   154,   155,   159,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,   177,   182,   183,
     184,   185,   189,   190,   195,   199,   200,   206,   207,   208,
     209,   213,   214,   218,   219,   220,   221,   222,   223,   227,
     228,   232,   233,   237,   238,   239,   243,   247,   248,   249,
     250,   251,   252,   256,   257,   261,   262,   266,   270,   271,
     272,   273,   277,   278,   282,   289,   290,   291,   292,   293,
     297,   298,   302,   303,   307,   308,   309
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
#line 1859 "yy_parser_generated.cpp"

#line 311 "c.y"


void ffi::YYParser::error(const ffi::Location &loc, const std::string &message) {
  throw ParsingDriver::ParseError{loc, message};
}
