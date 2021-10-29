// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

// Based on http://www.quut.com/c/ANSI-C-grammar-y-2011.html

%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.0"

%defines
%define api.parser.class {YYParser}
%define api.token.prefix {C_TOKEN_}
%define api.namespace {ffi}
%define parse.error verbose

%parse-param { ffi::Lexer &lexer }
%parse-param { ffi::ParsingDriver &driver }

%locations
%define api.location.type {ffi::Location}

%code requires
{
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
}

%code top
{
    #include "compiler/ffi/c_parser/lexer.h"
    #include "compiler/ffi/c_parser/parsing_driver.h"
    #include "compiler/ffi/c_parser/yy_parser_generated.hpp"

    #define yylex(sym, loc) lexer.get_next_token((sym), (loc))
}

%token END 0 "end of file"

%token COMMA ",";
%token SEMICOLON ";";
%token LBRACE "{";
%token RBRACE "}";
%token LPAREN "(";
%token RPAREN ")";
%token LBRACKET "[";
%token RBRACKET "]";
%token MUL "*";
%token EQUAL "=";
%token PLUS "+";
%token MINUS "-";

%token IDENTIFIER INT_CONSTANT FLOAT_CONSTANT STRING_LITERAL SIZEOF
%token PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN TYPEDEF_NAME

%token TYPEDEF INLINE EXTERN STATIC AUTO REGISTER THREAD_LOCAL
%token BOOL CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE VOID
%token INT8 INT16 INT32 INT64
%token UINT8 UINT16 UINT32 UINT64
%token SIZE_T
%token STRUCT UNION ENUM ELLIPSIS

%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%union {
  string_span str_;
  int int_;
  FFIType *type_;
}

%type < str_ > typedef_name_or_identifier;
%type < str_ > IDENTIFIER;
%type < str_ > TYPEDEF_NAME;
%type < str_ > INT_CONSTANT;
%type < str_ > FLOAT_CONSTANT;

%type < int_ > int_value;

%type < type_ > declaration;
%type < type_ > declaration_specifiers;
%type < type_ > type_qualifier;
%type < type_ > type_qualifier_list;
%type < type_ > type_specifier;
%type < type_ > base_type_specifier;
%type < type_ > init_declarator_list;
%type < type_ > init_declarator;
%type < type_ > declarator;
%type < type_ > direct_declarator;
%type < type_ > abstract_declarator;
%type < type_ > parameter_type_list;
%type < type_ > parameter_list;
%type < type_ > parameter_declaration;
%type < type_ > pointer;
%type < type_ > struct_or_union_specifier;
%type < type_ > specifier_qualifier_list;
%type < type_ > struct_declaration;
%type < type_ > struct_declarator_list;
%type < type_ > struct_declarator;
%type < type_ > struct_declaration_list;
%type < type_ > enum_specifier;
%type < type_ > enumerator_list;
%type < type_ > enumerator;

%start translation_unit

%%

translation_unit
  : external_declaration
  | translation_unit external_declaration
  ;

// Normally, external_declaration includes function_definition, but we don't care about them
external_declaration
  : declaration { driver.add_type($1); }
  | typedef_declaration {}
  | EXTERN declaration { driver.add_type($2); }
  | STATIC declaration { driver.add_type($2); }
  | AUTO declaration { driver.add_type($2); }
  | REGISTER declaration { driver.add_type($2); }
  ;

typedef_declaration
  : TYPEDEF specifier_qualifier_list declarator SEMICOLON { driver.add_typedef($2, $3); }
  ;

declaration
  : declaration_specifiers SEMICOLON { $$ = $1; }
  | declaration_specifiers init_declarator_list SEMICOLON { $$ = driver.combine(DeclarationSpecifiers{$1}, InitDeclaratorList{$2}); }
  ;

declaration_specifiers
  : type_specifier { $$ = $1; }
  | type_specifier declaration_specifiers { $$ = driver.combine(TypeSpecifier{$1}, DeclarationSpecifiers{$2}); }
  | type_qualifier { $$ = $1; }
  | type_qualifier declaration_specifiers { $$ = driver.combine(TypeQualifier{$1}, DeclarationSpecifiers{$2}); }
  ;

type_qualifier
  : CONST { $$ = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Const); }
  | VOLATILE { $$ = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Volatile); }
  ;

base_type_specifier
  : VOID { $$ = driver.make_simple_type(FFITypeKind::Void); }
  | CHAR { $$ = driver.make_simple_type(FFITypeKind::Char); }
  | BOOL { $$ = driver.make_simple_type(FFITypeKind::Bool); }
  | FLOAT { $$ = driver.make_simple_type(FFITypeKind::Float); }
  | DOUBLE { $$ = driver.make_simple_type(FFITypeKind::Double); }
  | INT { $$ = driver.make_simple_type(FFITypeKind::_int); }
  | LONG { $$ = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Long); }
  | SHORT { $$ = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Short); }
  | SIGNED { $$ = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Signed); }
  | UNSIGNED { $$ = driver.make_simple_type(FFITypeKind::_typeFlags, FFIType::Flag::Unsigned); }
  | INT8 { $$ = driver.make_simple_type(FFITypeKind::Int8); }
  | INT16 { $$ = driver.make_simple_type(FFITypeKind::Int16); }
  | INT32 { $$ = driver.make_simple_type(FFITypeKind::Int32); }
  | INT64 { $$ = driver.make_simple_type(FFITypeKind::Int64); }
  | UINT8 { $$ = driver.make_simple_type(FFITypeKind::Uint8); }
  | UINT16 { $$ = driver.make_simple_type(FFITypeKind::Uint16); }
  | UINT32 { $$ = driver.make_simple_type(FFITypeKind::Uint32); }
  | UINT64 { $$ = driver.make_simple_type(FFITypeKind::Uint64); }
  | SIZE_T { $$ = driver.make_simple_type(FFITypeKind::_size); }

  ;

type_specifier
  : base_type_specifier { $$ = $1; }
  | struct_or_union_specifier { $$ = $1; }
  | enum_specifier { $$ = $1; }
  | TYPEDEF_NAME { $$ = driver.get_aliased_type($1); }
  ;

init_declarator_list
  : init_declarator { $$ = driver.make_simple_type(FFITypeKind::_initDeclaratorList); $$->members.emplace_back($1); }
  | init_declarator_list COMMA init_declarator { $$ = $1; $$->members.emplace_back($3); }
  ;

// The `declarator '=' initializer` form is omitted here
init_declarator
  : declarator { $$ = $1; }
  ;

declarator
  : direct_declarator { $$ = $1; }
  | pointer direct_declarator { $$ = driver.combine(Pointer{$1}, DirectDeclarator{$2}); }
  ;

// We ignore type qualifiers (const/volatile) for the declarators
// as we only care about the CV-qualifiers of "locations" (pointed types)
pointer
  : MUL type_qualifier_list pointer { $$ = $3; $$->num++; }
  | MUL type_qualifier_list { $$ = driver.make_pointer(); }
  | MUL pointer { $$ = $2; $$->num++; }
  | MUL { $$ = driver.make_pointer(); }
  ;

type_qualifier_list
  : type_qualifier { $$ = driver.make_simple_type(FFITypeKind::_typesList); $$->members.emplace_back($1); }
  | type_qualifier_list type_qualifier { $$ = $1; $$->members.emplace_back($2); }
  ;

direct_declarator
  : IDENTIFIER { $$ = driver.make_simple_type(FFITypeKind::Name); $$->str = $1.to_string(); }
  | LPAREN declarator RPAREN { $$ = $2; }
  | direct_declarator LBRACKET RBRACKET { $$ = driver.make_pointer(); $$ = driver.combine(Pointer{$$}, DirectDeclarator{$1}); }
  | direct_declarator LBRACKET INT_CONSTANT RBRACKET { $$ = driver.make_array_declarator($1, $3); }
  | direct_declarator LPAREN parameter_type_list RPAREN { $$ = driver.make_function($1, $3); }
  | direct_declarator LPAREN RPAREN { $$ = driver.make_function($1, nullptr); }
  ;

parameter_type_list
  : parameter_list COMMA ELLIPSIS { $$ = $1; $$->members.emplace_back(driver.make_simple_type(FFITypeKind::_ellipsis)); }
  | parameter_list { $$ = $1; }
  ;

parameter_list
  : parameter_declaration { $$ = driver.make_simple_type(FFITypeKind::_typesList); $$->members.emplace_back($1); }
  | parameter_list COMMA parameter_declaration { $$ = $1; $$->members.emplace_back($3); }
  ;

parameter_declaration
  : declaration_specifiers declarator { $$ = driver.combine(DeclarationSpecifiers{$1}, Declarator{$2}); }
  | declaration_specifiers abstract_declarator { $$ = driver.combine(DeclarationSpecifiers{$1}, AbstractDeclarator{$2}); }
  | declaration_specifiers { $$ = $1; }
  ;

abstract_declarator
  : pointer { $$ = $1; }
  ;

struct_or_union_specifier
  : STRUCT LBRACE struct_declaration_list RBRACE { $$ = driver.make_struct_def($3); }
  | STRUCT typedef_name_or_identifier LBRACE struct_declaration_list RBRACE { $$ = driver.make_struct_def($4); $$->str = $2.to_string(); }
  | STRUCT typedef_name_or_identifier { $$ = driver.make_simple_type(FFITypeKind::Struct); $$->str = $2.to_string(); }
  | UNION LBRACE struct_declaration_list RBRACE { $$ = driver.make_union_def($3); }
  | UNION typedef_name_or_identifier LBRACE struct_declaration_list RBRACE { $$ = driver.make_union_def($4); $$->str = $2.to_string(); }
  | UNION typedef_name_or_identifier { $$ = driver.make_simple_type(FFITypeKind::Union); $$->str = $2.to_string(); }
  ;

typedef_name_or_identifier
  : IDENTIFIER { $$ = $1; }
  | TYPEDEF_NAME { $$ = $1; }
  ;

struct_declaration_list
  : struct_declaration { $$ = driver.make_simple_type(FFITypeKind::_typesList); $$->members.emplace_back($1); }
  | struct_declaration_list struct_declaration { $$ = $1; $$->members.emplace_back($2); }
  ;

struct_declaration
  : specifier_qualifier_list struct_declarator_list SEMICOLON { $$ = $2; $$->members.emplace_back($1); }
  ;

specifier_qualifier_list
  : type_specifier specifier_qualifier_list { $$ = driver.combine(TypeSpecifier{$1}, SpecifierQualifierList{$2}); }
  | type_specifier { $$ = $1; }
  | type_qualifier specifier_qualifier_list { $$ = driver.combine(TypeQualifier{$1}, SpecifierQualifierList{$2}); }
  | type_qualifier { $$ = $1; }
  ;

struct_declarator_list
  : struct_declarator { $$ = driver.make_simple_type(FFITypeKind::_structFieldList); $$->members.emplace_back($1); }
  | struct_declarator_list COMMA struct_declarator { $$ = $1; $$->members.emplace_back($3); }
  ;

struct_declarator
  : declarator { $$ = $1; }
  ;

// Note: we don't associate registered enum constants with EnumDef;
// it's not hard to do so, but we don't need that info right now,
// so we skip this step to avoid extra bookkeeping
enum_specifier
  : ENUM LBRACE enumerator_list RBRACE { $$ = driver.make_simple_type(FFITypeKind::EnumDef); }
  | ENUM LBRACE enumerator_list COMMA RBRACE { $$ = driver.make_simple_type(FFITypeKind::EnumDef); }
  | ENUM IDENTIFIER LBRACE enumerator_list RBRACE { $$ = driver.make_simple_type(FFITypeKind::EnumDef); $$->str = $2.to_string(); }
  | ENUM IDENTIFIER LBRACE enumerator_list COMMA RBRACE { $$ = driver.make_simple_type(FFITypeKind::EnumDef); $$->str = $2.to_string(); }
  | ENUM IDENTIFIER { $$ = driver.make_simple_type(FFITypeKind::Enum); $$->str = $2.to_string(); }
  ;

enumerator_list
  : enumerator { $$ = driver.make_simple_type(FFITypeKind::_enumList); driver.add_enumerator($$, $1); }
  | enumerator_list COMMA enumerator { $$ = $1; driver.add_enumerator($$, $3); }
  ;

enumerator
  : IDENTIFIER EQUAL int_value { $$ = driver.make_enum_member($1, $3); }
  | IDENTIFIER { $$ = driver.make_simple_type(FFITypeKind::_enumMember); $$->str = $1.to_string(); }
  ;

int_value
  : INT_CONSTANT { $$ = driver.stoi($1); }
  | MINUS INT_CONSTANT { $$ = -driver.stoi($2); }
  | PLUS INT_CONSTANT { $$ = driver.stoi($2); }

%%

void ffi::YYParser::error(const ffi::Location &loc, const std::string &message) {
  throw ParsingDriver::ParseError{loc, message};
}
