// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/dummy-visitor-methods.h"
#include "runtime/kphp_core.h"
#include "runtime/memory_usage.h"
#include "runtime/refcountable_php_classes.h"

template<class T>
struct C$FFI$CData: public refcountable_php_classes<C$FFI$CData<T>>, private DummyVisitorMethods {
  T c_value;

  const char *get_class() const noexcept { return "FFI\\CData"; }
  int get_hash() const noexcept { return 1945543994; }

  using DummyVisitorMethods::accept;
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
    c_value = nullptr;
  }
};

template<class T>
struct CDataRef {
  T *c_value;

  void accept(InstanceMemoryEstimateVisitor &visitor __attribute__((unused))) {}
  const char *get_class() const noexcept { return "FFI\\CDataRef"; }
  int get_hash() const noexcept { return -1965114283; }
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
T *ffi_c_value_ptr(CDataPtr<T> ptr) { return {ptr.c_value}; }

template<class T>
T *ffi_c_value_ptr(class_instance<C$FFI$CData<T>> &cdata) { return {&cdata->c_value}; }

template<class T>
T *ffi_c_value_ptr(const class_instance<C$FFI$CData<T>> &cdata) { return &cdata->c_value; }

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
  return {reinterpret_cast<const char*>(ffi_c_value_ptr(v)), static_cast<string::size_type>(size)};
}

template<class T>
int64_t f$FFI$$sizeof(CDataPtr<T> cdata __attribute__ ((unused))) {
  return sizeof(CDataPtr<T>);
}

template<class T>
int64_t f$FFI$$sizeof(const class_instance<C$FFI$CData<T>> &cdata __attribute__ ((unused))) {
  return sizeof(C$FFI$CData<T>::c_value);
}

template<class T>
int64_t f$FFI$$sizeof(const CDataRef<T> &ref) {
  return sizeof(*ref.c_value);
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

// count() for fixed-size arrays
template<typename T, int64_t ArraySize>
constexpr int64_t f$count(T (&)[ArraySize]) { return ArraySize; }

template<class T>
class_instance<C$FFI$CData<T>> ffi_new_cdata() {
  class_instance<C$FFI$CData<T>> cdata;
  cdata.alloc();
  return cdata;
}

template<class T>
CDataPtr<typename std::remove_pointer<T>::type> ffi_new_cdata_ptr() {
  return CDataPtr<typename std::remove_pointer<T>::type>::create(nullptr);
}

template<class ToType, class FromType>
CDataRef<ToType> ffi_cast_scalar(FromType &v) {
  return CDataRef<ToType>{(ToType*)(&v->c_value)};
}

template<class ToType, class FromType>
CDataPtr<typename std::remove_pointer<ToType>::type> ffi_cast(FromType *v) {
  return CDataPtr<typename std::remove_pointer<ToType>::type>{(ToType)(v)};
}

template<class ToType, class FromType>
CDataPtr<typename std::remove_pointer<ToType>::type> ffi_cast(CDataPtr<FromType> v) {
  return CDataPtr<typename std::remove_pointer<ToType>::type>{(ToType)(v.c_value)};
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

template<class T>
const T &ffi_array_get(const T *array, int64_t offset) {
  return array[offset];
}

template<class T>
T &ffi_array_get(T *array, int64_t offset) {
  return array[offset];
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
inline string ffi_c2php(const char *v) { return string(v); }
inline string ffi_c2php(char v) { return {1, v}; }

template<class T>
CDataPtr<T> ffi_c2php(T* v) {
  if (v == nullptr) {
    return CDataPtr<T>::create(reinterpret_cast<T*>(1));
  }
  return CDataPtr<T>::create(v);
}

template<class T> class_instance<C$FFI$CData<T>> ffi_c2php(T v) {
  auto cdata = ffi_new_cdata<T>();
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

template<class T>
auto ffi_php2c(class_instance<C$FFI$CData<T>> v, ffi_tag<C$FFI$CData<T>>) { return v->c_value; }

template<class T>
void *ffi_php2c(CDataPtr<T> v, ffi_tag<void*>) { return v.c_value; }

template<class T>
const void *ffi_php2c(CDataPtr<const T> v, ffi_tag<const void*>) { return v.c_value; }

template<class T>
T* ffi_php2c(CDataPtr<T> v, ffi_tag<C$FFI$CData<T*>>) { return v.c_value; }

template<class T>
const T* ffi_php2c(CDataPtr<T> v, ffi_tag<C$FFI$CData<const T*>>) { return v.c_value; }

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
T* ffi_vararg_php2c(CDataPtr<T> v) { return v.c_value; }

template<class T>
auto ffi_vararg_php2c(class_instance<C$FFI$CData<T>> v) { return v->c_value; }
