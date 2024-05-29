// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "kphp-core/class-instance/refcountable-php-classes.h"
#include "kphp-core/kphp_core.h"
#include "runtime/dummy-visitor-methods.h"
#include "runtime/memory_usage.h"

template<class T>
struct C$FFI$CData: public refcountable_php_classes<C$FFI$CData<T>>, private DummyVisitorMethods {
  T c_value;

  const char *get_class() const noexcept { return "FFI\\CData"; }
  int get_hash() const noexcept { return 1945543994; }

  using DummyVisitorMethods::accept;
};

template<class T>
struct CDataArray: public refcountable_php_classes<CDataArray<T>> {
  T *data{nullptr};
  int64_t len{0};

  explicit CDataArray(int64_t len)
    : data{static_cast<T*>(dl::allocate(sizeof(T) * len))}
    , len{len} {
    static_assert(std::is_trivial_v<T>, "CDataArray elements should be trivial");
    std::memset(data, 0, sizeof(T) * len);
  }

  CDataArray(T* data, int64_t len)
    : data{data}
    , len{len} {}

  CDataArray() = default;

  ~CDataArray() {
    if (data) {
      dl::deallocate(data, sizeof(T) * len);
    }
  }

  void accept(CommonMemoryEstimateVisitor&) {}
};

// Maybe CDataRef is enough for both field/array references,
// but CDataPtr is semantically different: it's an address that
// was obtained explicitly by FFI::addr();
// We may reconsider this separation later
//
// Unfortunately, there is a dirty trick we're using here to be
// compatible with PHP without any significant performance loss.
// When PHP returns nullptr from a C function, it returns it as
// a null zval (normal PHP null) instead of CDate initialized with null.
// KPHP, on the other hand, always returns CDataPtr<T> as CDataPtr<T>.
// To make f$is_null() work identically, we store an addr of 1
// when we want to mark the value as "PHP null", f$is_null will
// understand that and return true or false accordingly.
// Calling FFI::isNull on a CDataPtr<T> that is iniailized with
// c_value=1 is not a good practice as it's not something
// that would work in PHP (it will give a type error).
template<class T>
struct CDataPtr {
  T *c_value;

  CDataPtr(): c_value{nullptr} {}

  static CDataPtr<T> create(T *ptr) {
    CDataPtr<T> cdata;
    cdata.c_value = ptr;
    return cdata;
  }

  explicit CDataPtr(T *c_value): c_value{c_value} {}

  bool is_php_null() const noexcept {
    return c_value == reinterpret_cast<T*>(1);
  }

  // allow `T*` ptr to `void*` ptr conversion
  operator CDataPtr<void> () { return CDataPtr<void>(c_value); }

  // allow `T*` ptr to `const T*` ptr conversion
  operator CDataPtr<const T> () { return CDataPtr<const T>(c_value); }

  // construction FFI pointer from KPHP null value
  CDataPtr(Optional<bool> v) {
    if (!v.is_null()) {
      php_warning("FFI: initialized ptr with incompatible optional");
    }
    c_value = reinterpret_cast<T*>(1);
  }

  void accept(CommonMemoryEstimateVisitor&) {}
};

template<class T>
struct CDataRef {
  T *c_value;

  void accept(CommonMemoryEstimateVisitor &visitor __attribute__((unused))) {}
  const char *get_class() const noexcept { return "FFI\\CDataRef"; }
  int get_hash() const noexcept { return -1965114283; }
};

// CDataArrayRef is a non-owning pointer to a C array that keeps its length
template<class T>
struct CDataArrayRef {
  T *data;
  int64_t len;

  void accept(CommonMemoryEstimateVisitor&) {}
};

template<class T>
CDataPtr<T> ffi_addr(class_instance<C$FFI$CData<T>> &cdata) { return CDataPtr<T>::create(&cdata->c_value); }

template<class T>
CDataPtr<T> ffi_addr(const class_instance<C$FFI$CData<T>> &cdata) { return CDataPtr<T>::create(&cdata->c_value); }

template<class T>
CDataPtr<T*> ffi_addr(T* &ref) { return CDataPtr<T*>::create(&ref); }

template<class T>
CDataPtr<T> ffi_addr(T &ref) { return CDataPtr<T>::create(&ref); }

template<class T>
CDataPtr<T*> ffi_addr(CDataPtr<T> &ref) { return CDataPtr<T*>::create(&ref.c_value); }

template<class T>
CDataPtr<T> ffi_addr(class_instance<CDataArray<T>> &cdata) { return CDataPtr<T>::create(cdata->data); }

template<class T>
CDataPtr<T> ffi_addr(const class_instance<CDataArray<T>> &cdata) { return CDataPtr<T>::create(cdata->data); }

template<class T>
T *ffi_c_value_ptr(CDataPtr<T> ptr) {
  if (ptr.c_value == reinterpret_cast<T*>(1)) {
    return nullptr;
  }
  return ptr.c_value;
}

template<class T>
T *ffi_c_value_ptr(class_instance<C$FFI$CData<T>> &cdata) { return {&cdata->c_value}; }

template<class T>
T *ffi_c_value_ptr(const class_instance<C$FFI$CData<T>> &cdata) { return &cdata->c_value; }

template<class T>
T *ffi_c_value_ptr(const class_instance<CDataArray<T>> &cdata) { return cdata->data; }

template<class T>
T *ffi_c_value_ptr(CDataArrayRef<T> ref) { return ref.data; }

template<class T>
class_instance<C$FFI$CData<T>> ffi_clone_impl(const T &value) {
  class_instance<C$FFI$CData<T>> cloned;
  cloned.alloc();
  cloned->c_value = value;
  return cloned;
}

template<class T>
class_instance<C$FFI$CData<T>> ffi_clone(const class_instance<C$FFI$CData<T>> &other) {
  return ffi_clone_impl(other->c_value);
}

template<class T>
class_instance<C$FFI$CData<T>> ffi_clone(const CDataRef<T> &ref) {
  return ffi_clone_impl(*ref.c_value);
}

template<class T>
CDataPtr<T> ffi_clone(CDataPtr<T> ref) { return ref; }

// see ffi_env_instance
class FFIEnv {
public:
  struct SharedLib {
    const char *path = "";
    void *handle = nullptr;
  };

  struct Symbol {
    const char *name = "";
    void *ptr = nullptr;
  };

  // kphp runtime lib should be a static library;
  // since we can't reliably link -ldl statically, we expect a user
  // that requires FFI with dynamically loaded shared libs to
  // specify an extra linker flag; we expect the KPHP compiler
  // to generate the code that would populate this struct
  struct ExternFunctions {
    void* (*dlopen)(const char *path, int mode);
    void* (*dlsym)(void *handle, const char *symbol_name);
  };

  FFIEnv() = default;
  FFIEnv(int num_libs, int num_symbols);

  bool is_shared_lib_opened(int id);
  bool is_scope_loaded(int sym_offset);

  void open_shared_lib(int id);
  void load_scope_symbols(int lib_id, int sym_offset, int num_lib_symbols);
  void load_symbol(int lib_id, int dst_sym_id);

  ExternFunctions funcs;
  int num_libs;
  SharedLib *libs;
  int num_symbols;
  Symbol *symbols;
};

// FFI env is reused inside worker (it caches dlopen-ed libs and stuff like that)
extern FFIEnv ffi_env_instance;

struct C$FFI$Scope : public refcountable_empty_php_classes {
public:
  const char *get_class() const noexcept {
    return "FFI\\Scope";
  }
};

class_instance<C$FFI$Scope> f$FFI$$load(const string &filename);
class_instance<C$FFI$Scope> f$FFI$$scope(const string &scope_name);

template<class T>
string f$FFI$$string(const T &v) {
  return string(reinterpret_cast<const char*>(ffi_c_value_ptr(v)));
}

template<class T>
string f$FFI$$string(const T &v, int64_t size) {
  // note: this function causes a segfault on PHP when called with null data
  return {reinterpret_cast<const char*>(ffi_c_value_ptr(v)), static_cast<string::size_type>(size)};
}

template<class T>
int64_t f$FFI$$sizeof(CDataPtr<T> cdata __attribute__ ((unused))) {
  return sizeof(CDataPtr<T>);
}

template<class T>
int64_t f$FFI$$sizeof(const class_instance<CDataArray<T>> &a) {
  return sizeof(T) * a->len;
}

template<class T>
int64_t f$FFI$$sizeof(const CDataArrayRef<T> &a) {
  return sizeof(T) * a.len;
}

template<class T>
int64_t f$FFI$$sizeof(const class_instance<C$FFI$CData<T>> &cdata __attribute__ ((unused))) {
  return sizeof(C$FFI$CData<T>::c_value);
}

template<class T>
int64_t f$FFI$$sizeof(const CDataRef<T> &ref) {
  return sizeof(*ref.c_value);
}

inline CDataPtr<void> f$ffi_cast_addr2ptr(int64_t addr) {
  CDataPtr<void> ptr;
  ptr.c_value = reinterpret_cast<void*>(addr);
  return ptr;
}

inline int64_t f$ffi_cast_ptr2addr(CDataPtr<void> ptr) {
  return reinterpret_cast<int64_t>(ptr.c_value);
}

template<class T1, class T2>
int64_t f$FFI$$memcmp(T1 ptr1, T2 ptr2, int64_t size)  {
  int64_t result = std::memcmp(ffi_c_value_ptr(ptr1), ffi_c_value_ptr(ptr2), size);
  // contrary to what PHP documentation says, it actually only returns -1, 0 and 1 values,
  // so we need to adjust memcmp results here
  if (result < -1L) {
    return -1L;
  }
  return result > 1L ? 1L : result;
}

template<class T>
void f$ffi_memcpy_string(T dst, const string &src, int64_t size) {
  std::memcpy(ffi_c_value_ptr(dst), src.c_str(), size);
}

template<class T1, class T2>
void f$FFI$$memcpy(T1 dst, const T2 &src, int64_t size) {
  std::memcpy(ffi_c_value_ptr(dst), ffi_c_value_ptr(src), size);
}

template<class T>
void f$FFI$$memset(T dst, int64_t value, int64_t size) {
  std::memset(ffi_c_value_ptr(dst), value, size);
}

template<class T>
bool f$FFI$$isNull(T *ptr) { return ptr == nullptr; }

template<class T>
bool f$FFI$$isNull(CDataPtr<T> ptr) { return ptr.c_value == nullptr; }

template<class T>
void f$FFI$$free(const class_instance<C$FFI$CData<T>> &cdata) {
  cdata->set_refcnt(cdata->get_refcnt() - 1);
}

template<class T>
void f$FFI$$free(const class_instance<CDataArray<T>> &cdata) {
  cdata->set_refcnt(cdata->get_refcnt() - 1);
}

template<class T>
void f$FFI$$free(const CDataPtr<T> &ptr) {
  dl::deallocate(ptr.c_value, sizeof(T));
}

template<class T>
constexpr int64_t f$count(const class_instance<CDataArray<T>> &a) {
  return a->len;
}

template<class T>
constexpr int64_t f$count(const CDataArrayRef<T> &a) {
  return a.len;
}

template<class T>
class_instance<C$FFI$CData<T>> ffi_new_cdata(bool owned) {
  class_instance<C$FFI$CData<T>> cdata;
  cdata.alloc();
  if (!owned) {
    cdata->set_refcnt(cdata->get_refcnt() + 1);
  }
  return cdata;
}

template<class T>
auto ffi_new_cdata_ptr() {
  return CDataPtr<std::remove_pointer_t<T>>::create(nullptr);
}

template<class T>
class_instance<CDataArray<T>> ffi_new_cdata_array(int64_t len, bool owned) {
  if (unlikely(len < 0)) {
    php_critical_error("FFI::new(): negative array size");
  }
  if (unlikely(len == 0)) {
    php_critical_error("FFI::new(): zero array size");
  }
  class_instance<CDataArray<T>> cdata;
  cdata.alloc(len);
  if (!owned) {
    cdata->set_refcnt(cdata->get_refcnt() + 1);
  }
  return cdata;
}

template<class ToType, class FromType>
CDataRef<ToType> ffi_cast_scalar(FromType &v) {
  return CDataRef<ToType>{(ToType*)(&v->c_value)};
}

template<class ToType, class T>
auto ffi_cast_from_array(class_instance<CDataArray<T>> &a) {
  return CDataPtr<std::remove_pointer_t<ToType>>{(ToType)a->data};
}

template<class ToType, class T>
auto ffi_cast_to_array(T* data, int64_t len) {
  // TODO: make CDataArrayRef expressible via KPHP type system and return that?
  // otherwise we need to allocate a class_instance for every cast;
  // not a big problem right now, but we can do better

  if (unlikely(len < 0)) {
    php_critical_error("FFI::cast(): negative array size");
  }
  if (unlikely(len == 0)) {
    php_critical_error("FFI::cast(): zero array size");
  }

  class_instance<CDataArray<ToType>> cdata;
  cdata.alloc();
  cdata->len = len;
  cdata->data = static_cast<ToType*>(data);
  // cast never creates owning objects
  cdata->set_refcnt(cdata->get_refcnt() + 1);
  return cdata;
}

template<class ToType, class T>
auto ffi_cast_to_array(const CDataPtr<T> &ptr, int64_t len) {
  return ffi_cast_to_array<ToType>(ptr.c_value, len);
}

template<class ToType, class FromType>
auto ffi_cast(FromType *v) {
  return CDataPtr<std::remove_pointer_t<ToType>>{(ToType)(v)};
}

template<class ToType, class FromType>
auto ffi_cast(CDataPtr<FromType> v) {
  return CDataPtr<std::remove_pointer_t<ToType>>{(ToType)(v.c_value)};
}

template<class ToType, class FromType>
CDataRef<ToType> ffi_cast(CDataRef<FromType> v) {
  return CDataRef<ToType>{(ToType*)(v.c_value)};
}

template<class ToType, class FromType>
CDataRef<ToType> ffi_cast(FromType &v) {
  return CDataRef<ToType>{(ToType*)(&v)};
}

template<class ToType, class FromType>
CDataRef<ToType> ffi_cast(class_instance<C$FFI$CData<FromType>> &v) {
  return CDataRef<ToType>{(ToType*)(&v->c_value)};
}

inline void ffi_array_bound_check(int64_t len, int64_t index) {
  if (unlikely(index < 0 || index >= len)) {
    php_critical_error("C array index out of bounds");
  }
}

template<class T>
void ffi_array_set(const class_instance<CDataArray<T>> &a, int64_t index, T value) {
  ffi_array_bound_check(a->len, index);
  a->data[index] = value;
}

template<class T>
void ffi_array_set(const class_instance<CDataArray<T>> &a, int64_t index, std::nullptr_t) {
  ffi_array_bound_check(a->len, index);
  a->data[index] = nullptr;
}

template<class T>
void ffi_array_set(CDataArrayRef<T> a, int64_t index, T value) {
  ffi_array_bound_check(a.len, index);
  a.data[index] = value;
}

template<class T>
void ffi_array_set(CDataArrayRef<T> a, int64_t index, std::nullptr_t) {
  ffi_array_bound_check(a.len, index);
  a.data[index] = nullptr;
}

template<class T>
void ffi_array_set(CDataPtr<T> ptr, int64_t index, T value) {
  ptr.c_value[index] = value;
}

template<class T>
T& ffi_array_get(const class_instance<CDataArray<T>> &a, int64_t index) {
  ffi_array_bound_check(a->len, index);
  return a->data[index];
}

template<class T>
T& ffi_array_get(CDataArrayRef<T> a, int64_t index) {
  ffi_array_bound_check(a.len, index);
  return a.data[index];
}

template<class T>
T& ffi_array_get(CDataPtr<T> ptr, int64_t index) {
  return ptr.c_value[index];
}

class_instance<C$FFI$Scope> ffi_load_scope_symbols(class_instance<C$FFI$Scope> instance, int shared_lib_id, int sym_offset, int num_symbols);

inline bool ffi_c2php(bool v) { return v; }
inline int64_t ffi_c2php(uint8_t v) { return v; }
inline int64_t ffi_c2php(uint16_t v) { return v; }
inline int64_t ffi_c2php(uint32_t v) { return v; }
inline int64_t ffi_c2php(uint64_t v) { return v; }
inline int64_t ffi_c2php(int8_t v) { return v; }
inline int64_t ffi_c2php(int16_t v) { return v; }
inline int64_t ffi_c2php(int32_t v) { return v; }
inline int64_t ffi_c2php(int64_t v) { return v; }
inline double ffi_c2php(float v) { return v; }
inline double ffi_c2php(double v) { return v; }
inline string ffi_c2php(char v) { return {1, v}; }

inline Optional<string> ffi_c2php(const char *v) {
  if (v == nullptr) {
    return {};
  }
  return string(v);
}

template<class T>
CDataArrayRef<T> ffi_c2php_array(T *data, int64_t size) {
  return CDataArrayRef<T>{data, size};
}

template<class T>
CDataPtr<T> ffi_c2php(T *v) {
  if (v == nullptr) {
    return CDataPtr<T>::create(reinterpret_cast<T*>(1));
  }
  return CDataPtr<T>::create(v);
}

template<class T> class_instance<C$FFI$CData<T>> ffi_c2php(T v) {
  auto cdata = ffi_new_cdata<T>(true);
  cdata->c_value = v;
  return cdata;
}

template<class T>
CDataRef<T> ffi_c2php_ref(T &v) {
  return CDataRef<T>{&v};
}

template<class T>
struct ffi_tag {};

template<> struct ffi_tag<bool>{};
template<> struct ffi_tag<uint8_t>{};
template<> struct ffi_tag<uint16_t>{};
template<> struct ffi_tag<uint32_t>{};
template<> struct ffi_tag<uint64_t>{};
template<> struct ffi_tag<int8_t>{};
template<> struct ffi_tag<int16_t>{};
template<> struct ffi_tag<int32_t>{};
template<> struct ffi_tag<int64_t>{};
template<> struct ffi_tag<float>{};
template<> struct ffi_tag<double>{};
template<> struct ffi_tag<char>{};
template<> struct ffi_tag<const char*>{};
template<> struct ffi_tag<const void*>{};
template<> struct ffi_tag<void*>{};
template<class T> struct ffi_tag<C$FFI$CData<T>>{};

inline bool ffi_php2c(bool v, ffi_tag<bool>) { return v; }
inline uint8_t ffi_php2c(int64_t v, ffi_tag<uint8_t>) { return v; }
inline uint16_t ffi_php2c(int64_t v, ffi_tag<uint16_t>) { return v; }
inline uint32_t ffi_php2c(int64_t v, ffi_tag<uint32_t>) { return v; }
inline uint64_t ffi_php2c(int64_t v, ffi_tag<uint64_t>) { return v; }
inline int8_t ffi_php2c(int64_t v, ffi_tag<int8_t>) { return v; }
inline int16_t ffi_php2c(int64_t v, ffi_tag<int16_t>) { return v; }
inline int32_t ffi_php2c(int64_t v, ffi_tag<int32_t>) { return v; }
inline int64_t ffi_php2c(int64_t v, ffi_tag<int64_t>) { return v; }
inline float ffi_php2c(double v, ffi_tag<float>) { return static_cast<float>(v); }
inline double ffi_php2c(double v, ffi_tag<double>) { return v; }
inline const char* ffi_php2c(const string &v, ffi_tag<const char*>) { return v.c_str(); }
inline const void* ffi_php2c(const string &v, ffi_tag<const void*>) { return v.c_str(); }

inline const char* ffi_php2c(const Optional<string> &v, ffi_tag<const char*>) {
  return v.has_value() ? v.val().c_str() : nullptr;
}

// some FFI types allow nulls as their values
inline bool ffi_php2c(Optional<bool> v, ffi_tag<bool>) { return v.is_null() ? false : v.val(); }
inline uint8_t ffi_php2c(Optional<int64_t> v, ffi_tag<uint8_t>) { return v.is_null() ? 0 : v.val(); }
inline uint16_t ffi_php2c(Optional<int64_t> v, ffi_tag<uint16_t>) { return v.is_null() ? 0 : v.val(); }
inline uint32_t ffi_php2c(Optional<int64_t> v, ffi_tag<uint32_t>) { return v.is_null() ? 0 : v.val(); }
inline uint64_t ffi_php2c(Optional<int64_t> v, ffi_tag<uint64_t>) { return v.is_null() ? 0 : v.val(); }
inline int8_t ffi_php2c(Optional<int64_t> v, ffi_tag<int8_t>) { return v.is_null() ? 0 : v.val(); }
inline int16_t ffi_php2c(Optional<int64_t> v, ffi_tag<int16_t>) { return v.is_null() ? 0 : v.val(); }
inline int32_t ffi_php2c(Optional<int64_t> v, ffi_tag<int32_t>) { return v.is_null() ? 0 : v.val(); }
inline int64_t ffi_php2c(Optional<int64_t> v, ffi_tag<int64_t>) { return v.is_null() ? 0 : v.val(); }
inline float ffi_php2c(Optional<double> v, ffi_tag<float>) { return v.is_null() ? 0 : static_cast<float>(v.val()); }
inline double ffi_php2c(Optional<double> v, ffi_tag<double>) { return v.is_null() ? 0 : v.val(); }

template<class T>
auto ffi_php2c(class_instance<C$FFI$CData<T>> v, ffi_tag<C$FFI$CData<T>>) { return v->c_value; }

inline const char* ffi_php2c(CDataPtr<char> v, ffi_tag<const char*>) { return ffi_c_value_ptr(v); }
inline const char* ffi_php2c(CDataPtr<const char> v, ffi_tag<const char*>) { return ffi_c_value_ptr(v); }

template<class T>
void *ffi_php2c(CDataPtr<T> v, ffi_tag<void*>) { return ffi_c_value_ptr(v); }

template<class T>
const void *ffi_php2c(CDataPtr<T> v, ffi_tag<const void*>) { return ffi_c_value_ptr(v); }

template<class T>
const void *ffi_php2c(CDataPtr<const T> v, ffi_tag<const void*>) { return ffi_c_value_ptr(v); }

template<class T>
T* ffi_php2c(CDataPtr<T> v, ffi_tag<C$FFI$CData<T*>>) { return ffi_c_value_ptr(v); }

template<class T>
const T* ffi_php2c(CDataPtr<T> v, ffi_tag<C$FFI$CData<const T*>>) { return ffi_c_value_ptr(v); }

template<class T>
const T* ffi_php2c(CDataPtr<const T> v, ffi_tag<C$FFI$CData<const T*>>) { return ffi_c_value_ptr(v); }

// this is a special overloading for things like php2c(null);
// note: we compile op_null as Optional<bool>{}
// TODO: not needed?
template<class T>
T ffi_php2c(Optional<bool> v, ffi_tag<C$FFI$CData<T>>) {
  php_assert(v.is_null());
  return nullptr;
}

inline char ffi_php2c(const string &v, ffi_tag<char>) {
  if (unlikely(v.size() != 1)) {
    php_warning("FFI: incompatible types when assigning to type 'char' from PHP 'string'");
    return 0;
  }
  return v[0];
}

// Functions below govern the php2c conversions for variadic arguments.
// Note: we don't do bool->int promotions here as it will be performed
// by the C++ compiler on its own.
// The only real difference here is that functions don't get a second tag
// argument, they infer the matching C type based on the input argument.

inline bool ffi_vararg_php2c(bool v) { return v; }
inline int64_t ffi_vararg_php2c(int64_t v) { return v; }
inline double ffi_vararg_php2c(double v) { return v; }
inline const char *ffi_vararg_php2c(const string &s) { return s.c_str(); }

template<class T>
T* ffi_vararg_php2c(CDataPtr<T> v) { return ffi_c_value_ptr(v); }

template<class T>
auto ffi_vararg_php2c(class_instance<C$FFI$CData<T>> v) { return v->c_value; }
