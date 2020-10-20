---types---
/////
//
// Serialized binary TL-schema in vkext format
//
/////

// Flags description:
// FLAG_BARE                = (1 << 0)          ---          Is type expression bare
// FLAG_NOCONS              = (1 << 1)          ---          unknown
// FLAG_OPT_VAR             = (1 << 17)         ---          Is argument optional, e.g. wrapped to '{}'
// FLAG_EXCL                = (1 << 18)         ---          Is argument a forwarded function (via !)
// FLAG_OPT_FIELD           = (1 << 20)         ---          Is argument hidden with fields mask
// FLAG_NOVAR               = (1 << 21)         ---          unknown
// FLAG_DEFAULT_CONSTRUCTOR = (1 << 25)         ---          Does type have a default constructor, e.g. constructor that will be used if no magic is presented.
//                                                           If it does, default constructor is last one
// FLAG_FORWARDED           = (1 << 27)         ---          Is type expression a result of forwarded (via !) function (used only at vkext)

// The following flags make sense only for functions:
// COMBINATOR_FLAG_READ     = 1                 ---          Does function represent read query
// COMBINATOR_FLAG_WRITE    = 2                 ---          Does function represent write query
// COMBINATOR_FLAG_INTERNAL = 4                 ---          Is function used only for internal engines interconnection
// COMBINATOR_FLAG_KPHP     = 8                 ---          Is function processed by kphp rpc server

tls.schema_v2 version:int date:int types_num:# types:types_num*[tls.Type] constructor_num:# constructors:constructor_num*[tls.Combinator] functions_num:# functions:functions_num*[tls.Combinator] = tls.Schema;

tls.schema_v3 version:int date:int types_num:# types:types_num*[tls.Type] constructor_num:# constructors:constructor_num*[tls.Combinator] functions_num:# functions:functions_num*[tls.Combinator] = tls.Schema;

tls.schema_v4 version:int date:int types_num:# types:types_num*[tls.Type] constructor_num:# constructors:constructor_num*[tls.Combinator] functions_num:# functions:functions_num*[tls.Combinator] = tls.Schema;

tls.type name:int id:string constructors_num:int flags:int arity:int params_type:long = tls.Type;
tls.combinator name:int id:string type_name:int left:tls.CombinatorLeft right:tls.CombinatorRight = tls.Combinator;
tls.combinator_v4 name:int id:string type_name:int left:tls.CombinatorLeft right:tls.CombinatorRight flags:int = tls.Combinator;
tls.combinatorLeftBuiltin = tls.CombinatorLeft;
tls.combinatorLeft args_num:# args:args_num*[tls.Arg] = tls.CombinatorLeft;

tls.combinatorRight value:tls.TypeExpr = tls.CombinatorRight;

tls.arg id:string flags:# var_num:flags.1?int exist_var_num:flags.2?int exist_var_bit:flags.2?int type:tls.TypeExpr = tls.Arg;
// current version of TL-schema looks like this:
// tls.arg id:string flags:# var_num:flags.2?int exist_var_num:flags.1?int exist_var_bit:flags.1?int type:tls.TypeExpr = tls.Arg;

tls.exprType#ecc9da78 expr:tls.TypeExpr = tls.Expr;
tls.exprNat#dcb49bd8 expr:tls.NatExpr = tls.Expr;

tls.natConst value:int = tls.NatExpr;
tls.natVar dif:int var_num:int = tls.NatExpr;

tls.typeVar var_num:int flags:int = tls.TypeExpr;
tls.array multiplicity:tls.NatExpr args_num:# args:args_num*[tls.Arg] = tls.TypeExpr;
tls.typeExpr name:int flags:int children_num:# children:children_num*[tls.Expr] = tls.TypeExpr;
---functions---
