// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

// php.h sometimes make assert nothing.

extern "C" {

#include <php.h>

// order of include matters :(
#include <ext/standard/php_var.h>
#include <php_ini.h>
#include <zend.h>
}

#undef NDEBUG
#include <assert.h>

#define VK_LEN_T                           size_t
#define VK_STR_API_LEN(l)                  (l)
#define VK_RETURN_STRINGL_DUP(v, l)        RETURN_STRINGL(v, l)
#define VK_RETVAL_STRINGL_DUP(value, len)  RETVAL_STRINGL(value,len)
#define VK_RETURN_STRING_DUP(v)            RETURN_STRING(v)
#define VK_RETVAL_STRING_DUP(s)            RETVAL_STRING(s)
#define VK_ZVAL_STRING_DUP(_z, _s)         ZVAL_STRING(_z,_s)

#define VK_RETVAL_STRINGL_NOD(value, len) {           \
    RETVAL_STRINGL(value, len);                       \
    efree(value);                                     \
} while(0)

#define VK_ZVAL_STRINGL_NOD(dst, s, l) {              \
    ZVAL_STRINGL(dst,s,l);                            \
    efree(s);                                         \
} while(0)

#define VK_RETVAL_STRING_NOD(_s) {                    \
    RETVAL_STRING(_s);                                \
    efree(_s);                                        \
} while(0)

#define VK_RETURN_STRINGL_NOD(_value, _len) {         \
    VK_RETVAL_STRINGL_NOD(_value, _len)               \
    return;                                           \
} while(0)

#define VK_RETURN_STRING_NOD(_s) {                    \
  RETVAL_STRING(_s);                                  \
  efree(_s);                                          \
  return;                                             \
} while(0)

#define vk_add_assoc_string_dup(arr, s, val)                      add_assoc_string(arr, s, val)
#define vk_add_index_string_dup(arr, num, val)                    add_index_string(arr, num, val)
#define vk_add_assoc_stringl_dup(arg, key, key_len, str, length)  add_assoc_stringl_ex(arg, key, key_len, str, length)
#define vk_add_next_index_stringl(arg, str, length)               add_next_index_stringl((arg), (str), (length))

#define vk_add_assoc_stringl_nod(arg, key, key_len, str, length) {  \
  add_assoc_stringl_ex(arg, key, key_len, str, length);             \
  efree(str);                                                       \
  } while(0)

#define vk_add_assoc_zval_ex_nod(arr, key, len, val) {              \
  add_assoc_zval_ex (arr, key, len, val);                           \
  efree(val);                                                       \
  } while(0)

#define vk_add_assoc_zval_nod(arr, key, val) vk_add_assoc_zval_ex_nod(arr, key,  strlen(key), val)

#define vk_add_index_zval_nod(arr, num, val) {                      \
  add_index_zval (arr, num, val);                                   \
  efree(val);                                                       \
  } while(0)

#define VK_HASH_KEY_LEN(l) (l)

#define VK_ALLOC_ZVAL(z)                \
  (z) = (zval*)emalloc(sizeof(zval))

#define VK_INIT_ZVAL(z)                 \
  memset(&z, 0, sizeof(zval));

#define VK_ALLOC_INIT_ZVAL(zp)          \
  VK_ALLOC_ZVAL(zp);                    \
  VK_INIT_ZVAL(*zp);

#define VK_ZVAL_API_ARRAY                   zval
#define VK_ZVAL_ARRAY_TO_API_P(z)           (&(z))
#define VK_Z_STRVAL_P(p)                    Z_STRVAL_P(p)
#define VK_ZVAL_API_ARRAY_EL_TYPE(el)       (Z_TYPE_P(&(el)))
#define VK_IS_BOOL_T_CASE                   IS_FALSE...IS_TRUE
#define VK_ZVAL_IS_TRUE(pzv)                Z_TYPE_P(pzv) == IS_TRUE
#define VK_ZVAL_API_P                       zval*
#define Z_PP_TO_ZAPI_P(zp)                  (*(zp))
#define VK_ZVAL_API_P_STR(pz)               Z_STRVAL_P(pz)
#define VK_ZVAL_API_TO_ZVALP(pz)            (pz)

#define VK_Z_BVAL_P(pz)       (Z_TYPE_P((pz)) == IS_TRUE)

#define VK_Z_API_TYPE(pz)                   Z_TYPE_P(pz)
#define VK_Z_API_ARRVAL(pz)                 Z_ARRVAL_P(pz)
#define VK_ZVAL_API_TO_ZVAL(pz)             (*(pz))
#define VK_ZVAL_API_REF(pz)                 pz
#define vk_z_string                         zend_string
#define VK_ZARG_T                           zval
#define VK_MAKE_ZARG(za)                    zval za
#define VK_INIT_ZARG(za)                    za
#define VK_ZARG_TO_P(za)                    (&(za))
#define VK_ZP_TO_ZARG(zp)                   (*(zp))
#define VK_ZARG_FREE(za)                    zval_dtor(&(za))


#define NEW_INIT_Z_STR_P(name)                            vk_z_string* name
#define ZEND_HASH_NUM_APPLY_CN(_pht)                      (_pht)->u.v.nApplyCount
#define VK_ZEND_HASH_FOREACH_KEY(_ht, _h, _pzs)           ZEND_HASH_FOREACH_KEY((_ht), (_h), (_pzs))
#define VK_ZEND_HASH_FOREACH_VAL(_ht, _val)               ZEND_HASH_FOREACH_VAL((_ht), (_val))
#define VK_ZEND_HASH_FOREACH_KEY_VAL(_ht, _h, _pzs, _val) ZEND_HASH_FOREACH_KEY_VAL((_ht), (_h), (_pzs), (_val))
#define VK_ZEND_HASH_FOREACH_END()                        ZEND_HASH_FOREACH_END()
#define VK_ZVALP_TO_API_ZVALP(pz)                         (pz)
#define VK_TRY_ADDREF_P(pz)                               Z_TRY_ADDREF_P(pz)
#define VK_ZAPI_TRY_ADDREF_P(pz)                          VK_TRY_ADDREF_P(pz)

#define VK_ZEND_HASHES_INTERATE_VAL(_ht, _ppos, _pval)                                              \
    for ( zend_hash_internal_pointer_reset_ex(_ht, _ppos);                                          \
          (((_pval) = zend_hash_get_current_data_ex((_ht), (_ppos))) != NULL);                      \
          zend_hash_move_forward_ex((_ht), (_ppos)))  {

#define VK_ZEND_HASHES_FOREACH_KEY_VAL(_ht, _h, _pzs, _val)                                         \
    do {                                                                                            \
    HashPosition _pos; (_h) = 0;                                                                    \
    VK_ZEND_HASHES_INTERATE_VAL((_ht), &_pos, (_val))                                               \
    (_pzs) = (zend_hash_get_current_key_ex((_ht), &(_pzs), &(_h), &_pos) == HASH_KEY_IS_STRING)?    \
             (_pzs) : NULL;


static zend_always_inline VK_ZVAL_API_P vk_zend_hash_find(HashTable *ht, const char *key, size_t len) {
  zend_string *key_str = zend_string_init(key, len, 0);
  VK_ZVAL_API_P result = zend_hash_find(ht, key_str);
  zend_string_release(key_str);
  return result;
}

static zend_always_inline int vk_zend_hash_exists(HashTable *ht, const char *key, size_t len) {
  return zend_hash_str_exists(ht, key, len);
}

static zend_always_inline VK_ZVAL_API_P vk_zend_hash_index_find(HashTable *ht, zend_ulong idx) {
  return zend_hash_index_find(ht, idx);
}

static zend_always_inline int vk_is_bool(zval *z) {
  return Z_TYPE_P(z) == IS_FALSE || Z_TYPE_P(z) == IS_TRUE;
}

static zend_always_inline int vk_get_bool_value(zval *b) {
  switch (Z_TYPE_P(b)) {
    case IS_FALSE:
      return 0;
    case IS_TRUE:
      return 1;
    default:
      assert(0 && "unexpected zend type in vk_get_bool_value");
      return 0;
  }
}

static zend_always_inline zend_class_entry *vk_get_class(const char *class_name) {
  zend_string *zend_str_class_name = zend_string_init(class_name, strlen(class_name), 0);
  zend_class_entry *res_class_entry = zend_lookup_class(zend_str_class_name);
  zend_string_release(zend_str_class_name);
  if (!res_class_entry) {
    fprintf(stderr, "Class '%s' not found\n", class_name);
    assert(res_class_entry);
  }
  return res_class_entry;
}

static zend_always_inline void vk_get_class_name(const zval *object, char *dst) {
  zend_string *zend_str_class_name = Z_OBJ_HANDLER_P(object, get_class_name)(Z_OBJ_P(object));
  strcpy(dst, zend_str_class_name->val);
  zend_string_release(zend_str_class_name);
}

static zend_always_inline zval *vk_zend_read_public_property(zval *object, const char *prop_name) {
#if PHP_MAJOR_VERSION >= 8
  return zend_read_property(NULL, Z_OBJ(*object), prop_name, strlen(prop_name), 0, NULL);
#else
  return zend_read_property(NULL, object, prop_name, strlen(prop_name), 0, NULL);
#endif
}

static zend_always_inline void vk_zend_update_public_property_dup(zval *object, const char *prop_name, zval *value) {
#if PHP_MAJOR_VERSION >= 8
  zend_update_property(NULL, Z_OBJ(*object), prop_name, strlen(prop_name), value);
#else
  zend_update_property(NULL, object, prop_name, strlen(prop_name), value);
#endif
}

static zend_always_inline void vk_zend_update_public_property_nod(zval *object, const char *prop_name, zval *value) {
#if PHP_MAJOR_VERSION >= 8
  zend_update_property(NULL, Z_OBJ(*object), prop_name, strlen(prop_name), value);
#else
  zend_update_property(NULL, object, prop_name, strlen(prop_name), value);
#endif
  zval_ptr_dtor(VK_ZVALP_TO_API_ZVALP(value));
  efree(value);
}

static zend_always_inline void vk_zend_update_public_property_string(zval *object, const char *prop_name, const char *value) {
#if PHP_MAJOR_VERSION >= 8
  zend_update_property_stringl(NULL, Z_OBJ(*object), prop_name, strlen(prop_name), value, strlen(value));
#else
  zend_update_property_stringl(NULL, object, prop_name, strlen(prop_name), value, strlen(value));
#endif
}

static zend_always_inline void vk_zend_update_public_property_long(zval *object, const char *prop_name, zend_long value) {
#if PHP_MAJOR_VERSION >= 8
  zend_update_property_long(NULL, Z_OBJ(*object), prop_name, strlen(prop_name), value);
#else
  zend_update_property_long(NULL, object, prop_name, strlen(prop_name), value);
#endif
}

static zend_always_inline void vk_zend_call_known_instance_method(zval *object,
                                                                    const char * name, size_t name_len, zval *retval_ptr,
                                                                    uint32_t param_count, zval *params) {
  zend_object* zobj = Z_OBJ_P(object);
  zend_string* method_name = zend_string_init(name, name_len, 0);
  zend_function* fun = Z_OBJ_HANDLER_P(object, get_method)(&zobj, method_name, 0);
  zend_string_release(method_name);
  if (!fun) {
    return; // retval stays UNDEF
  }
#if PHP_MAJOR_VERSION >= 8
  zend_call_known_instance_method(fun, zobj, retval_ptr, param_count, params);
#else
  zval method_name_zval;
  ZVAL_STR(&method_name_zval, method_name);
  call_user_function(NULL, object, &method_name_zval, retval_ptr, param_count, params);
#endif
}

#define ZAPI_TO_PP(az)          (&(az))
#define ZP_TO_API_P(az)         (az)
#define SMART_STRDATA(ss)       ((ss).s)
#define SMART_STRVAL(ss)        ((ss).s->val)
#define SMART_STRLEN(ss)        ((ss).s->len)
#define VK_ZSTR_VAL(zs)         ((zs)->val)
#define VK_ZP_IS_REF_TYPE(pz)   Z_ISREF_P(pz)
#define VK_ZP_DEREF(pz)         ZVAL_DEREF(pz)

#define VK_ZSTR_P_NON_EMPTY(pz)   ((pz) && (pz->val) && (pz->len))
#define VK_INCORRECT_FD           -1

#define VK_INI_INT(name) zend_ini_long(const_cast<char *>(name), sizeof(name)-1, 0)
#define VK_INI_FLT(name) zend_ini_double(const_cast<char *>(name), sizeof(name)-1, 0)
#define VK_INI_STR(name) zend_ini_string_ex(const_cast<char *>(name), sizeof(name)-1, 0, NULL)
#define VK_INI_BOOL(name) ((zend_bool) VK_INI_INT(name))

#if PHP_VERSION_ID >= 70300

#define VK_ZEND_HASH_APPLY_PROTECTION(h)                 !(GC_FLAGS(h) & GC_IMMUTABLE)
#define VK_ZEND_HASH_PROTECT_RECURSION(h)                GC_PROTECT_RECURSION(h)
#define VK_ZEND_HASH_UNPROTECT_RECURSION(h)              GC_UNPROTECT_RECURSION(h)
#define VK_ZEND_HASH_IS_RECURSIVE(h)                     GC_IS_RECURSIVE(h)

#else // PHP_VERSION_ID >= 70300

#define VK_ZEND_HASH_APPLY_PROTECTION(h)                 ZEND_HASH_APPLY_PROTECTION(h)
#define VK_ZEND_HASH_PROTECT_RECURSION(h)                ZEND_HASH_NUM_APPLY_CN(h)++
#define VK_ZEND_HASH_UNPROTECT_RECURSION(h)              ZEND_HASH_NUM_APPLY_CN(h)--
#define VK_ZEND_HASH_IS_RECURSIVE(h)                     (ZEND_HASH_NUM_APPLY_CN(h) >= 1)

#endif // PHP_VERSION_ID >= 70300
