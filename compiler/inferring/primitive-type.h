#pragma once

#include <string>

#define PTYPE_NAME_(id) (#id + 3)
#define PTYPE_NAME(id) PTYPE_NAME_(id)

//foreach_ptype.h contain calls of FOREACH_PTYPE(tp) for each primitive_type name.
//All new primitive types should be added to foreach_ptype.h
//All primitive type names must start with tp_
enum PrimitiveType {
#define FOREACH_PTYPE(tp) tp,

#include "../foreach_ptype.h"

  ptype_size
};

//interface to PrimitiveType
template<PrimitiveType T_ID>
inline const char *ptype_name();
const char *ptype_name(PrimitiveType tp);
PrimitiveType get_ptype_by_name(const std::string &s);
PrimitiveType type_lca(PrimitiveType a, PrimitiveType b);
bool can_store_bool(PrimitiveType tp);

#define PTYPE_NAME_FUNC(id) template <> inline const char *ptype_name <id> () {return PTYPE_NAME (id);}
#define FOREACH_PTYPE(tp) PTYPE_NAME_FUNC (tp);

#include "../foreach_ptype.h"

#undef PTYPE_NAME_FUNC
