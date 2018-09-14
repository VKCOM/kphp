/*** PrimitiveType ***/
#define PTYPE_NAME_(id) (#id + 3)
#define PTYPE_NAME(id) PTYPE_NAME_(id)

//define ptype_name specialization for each PrimitiveType

template<PrimitiveType T_ID>
const char *ptype_name() {
  return nullptr;
}

#define PTYPE_NAME_FUNC(id) template <> inline const char *ptype_name <id> () {return PTYPE_NAME (id);}
#define FOREACH_PTYPE(tp) PTYPE_NAME_FUNC (tp);

#include "foreach_ptype.h"

#undef PTYPE_NAME_FUNC

/*** Key ***/
inline bool operator<(const Key &a, const Key &b) {
  return a.id < b.id;
}

inline bool operator>(const Key &a, const Key &b) {
  return a.id > b.id;
}

inline bool operator<=(const Key &a, const Key &b) {
  return a.id <= b.id;
}

inline bool operator!=(const Key &a, const Key &b) {
  return a.id != b.id;
}

inline bool operator==(const Key &a, const Key &b) {
  return a.id == b.id;
}

/*** TypeData ***/
bool operator<(const TypeData::KeyValue &a, const TypeData::KeyValue &b) {
  return a.first < b.first;
}
