#pragma once

#include <msgpack.hpp>

#include "common/containers/final_action.h"

#include "runtime/critical_section.h"
#include "runtime/exception.h"
#include "runtime/interface.h"
#include "runtime/kphp_core.h"

namespace msgpack {
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
namespace adaptor {

// string
template<>
struct convert<string> {
  const msgpack::object &operator()(const msgpack::object &obj, string& res_s) const {
    if (obj.type != msgpack::type::STR) {
      throw msgpack::type_error();
    }
    res_s = string(obj.via.str.ptr, obj.via.str.size);

    return obj;
  }
};

template<>
struct pack<string> {
  template <typename Stream>
  packer<Stream> &operator()(msgpack::packer<Stream> &packer, const string &s) const noexcept {
    packer.pack_str(s.size());
    packer.pack_str_body(s.c_str(), s.size());
    return packer;
  }
};

// array<T>
template<class T>
struct convert<array<T>> {
  msgpack::object const &operator()(msgpack::object const &obj, array<T> &res_arr) const {
    if (obj.type == msgpack::type::ARRAY) {
      res_arr.reserve(obj.via.array.size, 0, true);

      for (size_t i = 0; i < obj.via.array.size; ++i) {
        res_arr.set_value(i, obj.via.array.ptr[i].as<T>());
      }

      return obj;
    }

    if (obj.type == msgpack::type::MAP) {
      array_size size(0, 0, false);
      run_callbacks_on_map(obj.via.map,
                           [&size](int32_t, msgpack::object &) { size.int_size++; },
                           [&size](const string &, msgpack::object &) { size.string_size++; });

      res_arr.reserve(size.int_size, size.string_size, size.is_vector);
      run_callbacks_on_map(obj.via.map,
                           [&res_arr](int32_t key, msgpack::object &value) { res_arr.set_value(key, value.as<T>()); },
                           [&res_arr](const string &key, msgpack::object &value) { res_arr.set_value(key, value.as<T>()); });

      return obj;
    }

    throw msgpack::unpack_error("couldn't recognize type of unpacking array");
  }

private:
  template<class IntCallbackT, class StrCallbackT>
  static void run_callbacks_on_map(const msgpack::object_map &obj_map, const IntCallbackT &on_integer, const StrCallbackT &on_string) {
    for (size_t i = 0; i < obj_map.size; ++i) {
      auto &key = obj_map.ptr[i].key;
      auto &value = obj_map.ptr[i].val;

      switch (key.type) {
        case msgpack::type::POSITIVE_INTEGER:
        case msgpack::type::NEGATIVE_INTEGER:
          on_integer(key.as<int32_t>(), value);
          break;
        case msgpack::type::STR:
          on_string(key.as<string>(), value);
          break;
        default:
          throw msgpack::unpack_error("expected string or integer in array unpacking");
      }
    }
  }
};

template<class T>
struct pack<array<T>> {
  template <typename Stream>
  packer<Stream>& operator()(msgpack::packer<Stream>& packer, const array<T>& arr) const noexcept {
    if (arr.is_vector()) {
      packer.pack_array(arr.count());
      for (const auto &it : arr) {
        packer.pack(it.get_value());
      }
      return packer;
    }

    packer.pack_map(arr.count());
    for (const auto &it : arr) {
      if (it.is_string_key()) {
        packer.pack(it.get_string_key());
      } else {
        packer.pack(it.get_int_key());
      }
      packer.pack(it.get_value());
    }

    return packer;
  }
};

// mixed
 template<>
 struct convert<var> {
   const msgpack::object &operator()(const msgpack::object &obj, var& v) const {
     switch (obj.type) {
       case msgpack::type::STR:
         v = obj.as<string>();
         break;
       case msgpack::type::ARRAY:
         v = obj.as<array<var>>();
         break;
       case msgpack::type::NEGATIVE_INTEGER:
       case msgpack::type::POSITIVE_INTEGER:
         v = obj.as<int32_t>();
         break;
       case msgpack::type::FLOAT32:
       case msgpack::type::FLOAT64:
         v = obj.as<double>();
         break;
       case msgpack::type::BOOLEAN:
         v = obj.as<bool>();
         break;
       case msgpack::type::MAP:
         v = obj.as<array<var>>();
         break;
       case msgpack::type::NIL:
         v = var{};
         break;
       default:
         throw msgpack::type_error();
     }

     return obj;
   }
 };

 template<>
 struct pack<var> {
   template <typename Stream>
   packer<Stream> &operator()(msgpack::packer<Stream> &packer, const var &v) const noexcept {
     switch (v.get_type()) {
       case var::type::NUL:
         packer.pack_nil();
         break;
       case var::type::BOOLEAN:
         packer.pack(v.as_bool());
         break;
       case var::type::INTEGER:
         packer.pack(v.as_int());
         break;
       case var::type::FLOAT:
         packer.pack(v.as_double());
         break;
       case var::type::STRING:
         packer.pack(v.as_string());
         break;
       case var::type::ARRAY:
         packer.pack(v.as_array());
         break;
       default:
         __builtin_unreachable();
     }

     return packer;
   }
 };

// Optional<T>
template<class T>
struct convert<Optional<T>> {
  const msgpack::object &operator()(const msgpack::object &obj, Optional<T>& v) const {
    switch (obj.type) {
      case msgpack::type::BOOLEAN: {
        v = obj.as<bool>();
        if (unlikely(obj.as<bool>())) {
          char err_msg[256];
          snprintf(err_msg, 256, "Expected false for unpacking Optional<%s>", typeid(T).name());
          throw msgpack::unpack_error(err_msg);
        }
        break;
      }
      case msgpack::type::NIL:
        v = Optional<T>{};
        break;
      default:
        v = obj.as<T>();
        break;
    }

    return obj;
  }
};

template<class T>
struct pack<Optional<T>> {
  template <typename Stream>
  packer<Stream> &operator()(msgpack::packer<Stream> &packer, const Optional<T> &v) const noexcept {
    switch (v.value_state()) {
      case OptionalState::has_value:
        packer.pack(v.val());
        break;
      case OptionalState::false_value:
        packer.pack_false();
        break;
      case OptionalState::null_value:
        packer.pack_nil();
        break;
      default:
        __builtin_unreachable();
    }

    return packer;
  }
};

// class_instance<T>
struct CheckInstanceDepth {
  static size_t depth;
  static constexpr size_t max_depth = 20;

  CheckInstanceDepth() {
    depth++;
    if (depth > max_depth) {
      THROW_EXCEPTION(new_Exception(string(__FILE__), __LINE__, string("maximum depth of nested instances exceeded")));
    }
  }

  CheckInstanceDepth(const CheckInstanceDepth&) = delete;
  CheckInstanceDepth& operator=(const CheckInstanceDepth&) = delete;

  ~CheckInstanceDepth() {
    depth--;
  }
};

template<class T>
struct convert<class_instance<T>> {
  const msgpack::object &operator()(const msgpack::object &obj, class_instance<T>& instance) const {
    switch (obj.type) {
      case msgpack::type::NIL:
        instance = class_instance<T>{};
        break;
      case msgpack::type::ARRAY:
        instance = class_instance<T>{}.alloc();
        obj.convert(*instance.get());
        break;
      default:
        throw msgpack::unpack_error("Expected NIL or ARRAY type for unpacking class_instance");
    }

    return obj;
  }
};

template<class T>
struct pack<class_instance<T>> {
  template <typename Stream>
  packer<Stream> &operator()(msgpack::packer<Stream> &packer, const class_instance<T> &instance) const noexcept {
    CheckInstanceDepth check_depth;
    CHECK_EXCEPTION(return packer);
    if (instance.is_null()) {
      packer.pack_nil();
    } else {
      packer.pack(*instance.get());
    }

    return packer;
  }
};

} // namespace adaptor
} // MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
} // namespace msgpack

template<class T>
inline string f$msgpack_serialize(const T &value) noexcept {
  auto clean_buffer = vk::finally(f$ob_end_clean);

  f$ob_start();
  php_assert(f$ob_get_length().has_value() && f$ob_get_length().val() == 0);
  msgpack::pack(*coub, value);
  CHECK_EXCEPTION(return {});

  return std::move(coub->str());
}

template<class InstanceClass>
inline string f$instance_serialize(const class_instance<InstanceClass> &instance) noexcept {
  msgpack::adaptor::CheckInstanceDepth::depth = 0;
  return f$msgpack_serialize(instance);
}

/**
 * this function works nice with POSIX signal despite of exceptions
 * due to malloc_replacement_guard exceptions will be allocated in script memory
 * and we don't need critical_section here.
 *
 * For better understanding exceptions please look through this article
 * https://monoinfinito.wordpress.com/series/exception-handling-in-c/
 */
template<class ResultType = var>
inline ResultType f$msgpack_deserialize(const string &buffer) noexcept {
  if (buffer.empty()) {
    return {};
  }

  const auto malloc_replacement_guard = make_malloc_replacement_with_script_allocator();
  try {
    msgpack::object_handle oh = msgpack::unpack(buffer.c_str(), buffer.size());
    msgpack::object obj = oh.get();

    return obj.as<ResultType>();
  } catch (msgpack::type_error &e) {
    THROW_EXCEPTION(new_Exception(string(__FILE__), __LINE__, string("Unknown type found during deserialization")));
  } catch (msgpack::unpack_error &e) {
    THROW_EXCEPTION(new_Exception(string(__FILE__), __LINE__, string(e.what())));
  } catch (...) {
    THROW_EXCEPTION(new_Exception(string(__FILE__), __LINE__, string("something went wrong in deserialization, pass it to KPHP|Team")));
  }

  return {};
}

template<class ResultClass>
inline ResultClass f$instance_deserialize(const string &buffer, const string&) noexcept {
  return f$msgpack_deserialize<ResultClass>(buffer);
}
