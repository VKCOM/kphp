// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/kphp-msgpack-tags.h"

#include <charconv>

#include "common/algorithms/string-algorithms.h"
#include "common/wrappers/fmt_format.h"

#include "compiler/data/class-data.h"
#include "compiler/data/var-data.h"
#include "compiler/kphp_assert.h"
#include "compiler/name-gen.h"
#include "compiler/phpdoc.h"
#include "compiler/type-hint.h"
#include "compiler/utils/string-utils.h"

/*
 * MsgPackEncoder::encode($obj) and MsgPackEncoder::decode($json_str, Obj::class) are controlled by @kphp-msgpack tags.
 *
 * `@kphp-msgpack attr = value` can be written above a field or a class.
 * Above fields, valid attrs are: rename, skip, etc.
 * Above classes, valid attrs are: skip_if_default, visibility_policy, etc. (that apply to all fields)
 * Values for a field override values for a class.
 *
 * Besides, a PHP programmer can create `class MyEncoder extends MsgPackEncoder`
 * and override some constants.
 * These constants are applied to all classes being encoded/decoded by this encoder,
 * though @kphp-msgpack above a particular class overrides settings for encoder.
 */

namespace kphp_msgpack {

struct KnownMsgPackAttr {
  MsgPackAttrType type;
  int attr_strlen;
  const char *attr_name;

  constexpr KnownMsgPackAttr(const char *attr_name, MsgPackAttrType type)
    : type(type)
    , attr_strlen(__builtin_strlen(attr_name))
    , attr_name(attr_name) {}
};

class AllMsgPackAttrs {
  static constexpr int N_ATTRS = 5;
  static const KnownMsgPackAttr ALL_ATTRS[N_ATTRS];

public:
  [[gnu::always_inline]] static MsgPackAttrType name2type(vk::string_view name) {
    for (const KnownMsgPackAttr &attr : ALL_ATTRS) {
      if (attr.attr_strlen == name.size() && !strncmp(name.data(), attr.attr_name, name.size())) {
        return attr.type;
      }
    }
    return MsgPackAttrType::msgpack_attr_unknown;
  }

  [[gnu::noinline]] static const char *type2name(MsgPackAttrType type) {
    for (const KnownMsgPackAttr &tag : ALL_ATTRS) {
      if (tag.type == type) {
        return tag.attr_name;
      }
    }
    return "@tag";
  }
};

const KnownMsgPackAttr AllMsgPackAttrs::ALL_ATTRS[] = {
  KnownMsgPackAttr("skip", msgpack_attr_skip),
  KnownMsgPackAttr("array_as_hashmap", msgpack_attr_array_as_hashmap),
  KnownMsgPackAttr("key", msgpack_attr_key),
  KnownMsgPackAttr("use_keys", msgpack_attr_use_keys),
  KnownMsgPackAttr("use_names", msgpack_attr_use_names),
//  KnownMsgPackAttr("rename", msgpack_attr_rename),
//  KnownMsgPackAttr("raw_string", msgpack_attr_raw_string),
//  KnownMsgPackAttr("required", msgpack_attr_required),
//  KnownMsgPackAttr("float_precision", msgpack_attr_float_precision),
//  KnownMsgPackAttr("skip_if_default", msgpack_attr_skip_if_default),
//  KnownMsgPackAttr("visibility_policy", msgpack_attr_visibility_policy),
//  KnownMsgPackAttr("rename_policy", msgpack_attr_rename_policy),
//  KnownMsgPackAttr("flatten", msgpack_attr_flatten),
//  KnownMsgPackAttr("fields", msgpack_attr_fields),
};

static constexpr int ATTRS_ALLOWED_FOR_FIELD =
  msgpack_attr_key |
  msgpack_attr_skip |
  msgpack_attr_array_as_hashmap |
//  msgpack_attr_raw_string |
//  msgpack_attr_required |
//  msgpack_attr_float_precision |
//  msgpack_attr_skip_if_default |
  0;
static constexpr int ATTRS_ALLOWED_FOR_CLASS =
  msgpack_attr_use_keys |
  msgpack_attr_use_names |
  //  msgpack_attr_float_precision |
//  msgpack_attr_skip_if_default |
//  msgpack_attr_visibility_policy |
//  msgpack_attr_rename_policy |
//  msgpack_attr_flatten |
//  msgpack_attr_fields |
  0;


//static bool does_type_hint_allow_null(const TypeHint *type_hint) noexcept {
//  if (const auto *as_optional = type_hint->try_as<TypeHintOptional>()) {
//    return as_optional->or_null;
//  } else if (const auto *as_pipe = type_hint->try_as<TypeHintPipe>()) {
//    return vk::any_of(as_pipe->items, does_type_hint_allow_null);
//  } else if (const auto *as_primitive = type_hint->try_as<TypeHintPrimitive>()) {
//    return vk::any_of_equal(as_primitive->ptype, tp_Null, tp_mixed, tp_any);
//  }
//
//  // note, that "A" doesn't allow null here, only "?A" does
//  return false;
//}

// @kphp-msgpack can be prefixed with 'for': `@kphp-msgpack for MyMsgPackEncoder attr = value`
// here we extract this MyMsgPackEncoder, resolving uses and leaving a pointer after it into str
static ClassPtr extract_for_encoder_from_kphp_msgpack_tag(FunctionPtr resolve_context, vk::string_view &str) {
  kphp_assert(str.starts_with("for "));

  str = vk::trim(str.substr(4));
  size_t pos_sp = str.find(' ');
  kphp_error_act(pos_sp != std::string::npos, "Nothing after @kphp-msgpack for", return {});

  std::string for_encoder_name = resolve_uses(resolve_context, static_cast<std::string>(str.substr(0, pos_sp)));
  ClassPtr for_encoder = G->get_class(for_encoder_name);
  kphp_error(for_encoder, fmt_format("Class {} not found after @kphp-msgpack for", for_encoder_name));

  str = vk::trim(str.substr(pos_sp));
  return for_encoder;
}

//static RenamePolicy parse_rename_policy(vk::string_view rhs) noexcept {
//  if (rhs == "snake_case") {
//    return RenamePolicy::snake_case;
//  }
//  if (rhs == "camelCase") {
//    return RenamePolicy::camel_case;
//  }
//  kphp_error(rhs == "none",
//             fmt_format("@kphp-msgpack '{}' should be none|snake_case|camelCase, got '{}'", AllMsgPackAttrs::type2name(msgpack_attr_rename_policy), rhs));
//  return RenamePolicy::none;
//}
//
//static VisibilityPolicy parse_visibility_policy(vk::string_view rhs) noexcept {
//  if (rhs == "all") {
//    return VisibilityPolicy::all;
//  }
//  kphp_error(rhs == "public",
//             fmt_format("@kphp-msgpack '{}' should be all|public, got '{}'", AllMsgPackAttrs::type2name(msgpack_attr_visibility_policy), rhs));
//  return VisibilityPolicy::public_only;
//}

//static int parse_float_precision(vk::string_view rhs) noexcept {
//  int res = 0;
//  auto err = std::from_chars(rhs.begin(), rhs.end(), res).ec;
//  kphp_error(err == std::errc{} && res >= 0,
//             fmt_format("@kphp-msgpack '{}' should be non negative integer, got '{}'", AllMsgPackAttrs::type2name(msgpack_attr_float_precision), rhs));
//  return res;
//}

static SkipFieldType parse_skip(vk::string_view rhs) noexcept {
  if (rhs.empty() || rhs == "true" || rhs == "1") {
    return SkipFieldType::always_skip;
  }
  if (rhs == "encode") {
    return SkipFieldType::skip_encode_only;
  }
  if (rhs == "decode") {
    return SkipFieldType::skip_decode_only;
  }
  kphp_error(rhs == "false" || rhs == "0",
             fmt_format("@kphp-msgpack '{}' should be empty or true|false|encode|decode, got '{}'", AllMsgPackAttrs::type2name(msgpack_attr_skip), rhs));
  return SkipFieldType::dont_skip;
}

//static vk::string_view parse_fields_delimited_by_comma(vk::string_view rhs) noexcept {
//  // convert "$id, $age" to ",id,age," for fast later search of substr ",{name},"
//  std::unordered_set<size_t> names_hashes;
//  auto *fields_delim = new std::string(",");
//  for (const auto &field_name : split_skipping_delimeters(rhs, "$, ")) {
//    *fields_delim += field_name;
//    *fields_delim += ',';
//    kphp_error(names_hashes.insert(vk::std_hash(field_name)).second,
//               fmt_format("@kphp-msgpack '{}' lists a duplicated item ${}", AllMsgPackAttrs::type2name(msgpack_attr_fields), field_name));
//  }
//  return fields_delim->c_str();
//}

static bool parse_bool_or_true_if_nothing(MsgPackAttrType attr_type, vk::string_view rhs) noexcept {
  if (rhs.empty() || rhs == "true" || rhs == "1") {
    return true;
  }
  kphp_error(rhs == "false" || rhs == "0",
             fmt_format("@kphp-msgpack '{}' should be empty or true|false, got '{}'", AllMsgPackAttrs::type2name(attr_type), rhs));
  return false;
}

static int parse_key_value(MsgPackAttrType attr_type, vk::string_view rhs) noexcept {
  int res = 0;
  auto err = std::from_chars(rhs.begin(), rhs.end(), res).ec;
  kphp_error(err == std::errc{} && res >= 0,
             fmt_format("@kphp-msgpack '{}' should be non negative integer, got '{}'", AllMsgPackAttrs::type2name(attr_type), rhs));
  return res;
}

//static vk::string_view parse_rename(vk::string_view rhs) noexcept {
//  kphp_error(std::none_of(rhs.begin(), rhs.end(), [](char c) { return c == '"' || c == '\\'; }),
//             fmt_format("quotes and backslashes are forbidden in @kphp-msgpack '{}'", AllMsgPackAttrs::type2name(msgpack_attr_rename)));
//  return rhs;
//}

static KphpMsgPackTag parse_kphp_msgpack_tag(FunctionPtr resolve_context, vk::string_view str) {
  ClassPtr for_encoder = str.starts_with("for ") ? extract_for_encoder_from_kphp_msgpack_tag(resolve_context, str) : ClassPtr{};

  size_t pos_eq = str.find('=');
  vk::string_view attr_name = pos_eq == std::string::npos ? str : vk::trim(str.substr(0, pos_eq));
  vk::string_view rhs = pos_eq == std::string::npos ? "" : vk::trim(str.substr(pos_eq + 1));

  MsgPackAttrType attr_type = AllMsgPackAttrs::name2type(attr_name);
  KphpMsgPackTag msgpack_tag{.attr_type = attr_type, .for_encoder = for_encoder, .use_keys = true}; // initialize union by 0

  switch (attr_type) {
//    case msgpack_attr_rename:
//      kphp_error_act(!rhs.empty(), "@kphp-msgpack 'rename' expected to have a value after '='", break);
//      msgpack_tag.rename = parse_rename(rhs);
//      break;
    case msgpack_attr_skip:
      msgpack_tag.skip = parse_skip(rhs);
      break;
    case msgpack_attr_array_as_hashmap:
      msgpack_tag.array_as_hashmap = parse_bool_or_true_if_nothing(attr_type, rhs);
      break;
    case msgpack_attr_use_keys:
      msgpack_tag.use_keys = parse_bool_or_true_if_nothing(attr_type, rhs);
      break;
    case msgpack_attr_use_names:
      msgpack_tag.use_names = parse_bool_or_true_if_nothing(attr_type, rhs);
      break;
    case msgpack_attr_key:
      msgpack_tag.key = parse_key_value(attr_type, rhs);
      break;
//    case msgpack_attr_raw_string:
//      msgpack_tag.raw_string = parse_bool_or_true_if_nothing(attr_type, rhs);
//      break;
//    case msgpack_attr_required:
//      msgpack_tag.required = parse_bool_or_true_if_nothing(attr_type, rhs);
//      break;
//    case msgpack_attr_float_precision:
//      kphp_error_act(!rhs.empty(), "@kphp-msgpack 'float_precision' expected to have a value after '='", break);
//      msgpack_tag.float_precision = parse_float_precision(rhs);
//      break;
//    case msgpack_attr_skip_if_default:
//      msgpack_tag.skip_if_default = parse_bool_or_true_if_nothing(attr_type, rhs);
//      break;
//    case msgpack_attr_visibility_policy:
//      kphp_error_act(!rhs.empty(), "@kphp-msgpack 'visibility_policy' expected to have a value after '='", break);
//      msgpack_tag.visibility_policy = parse_visibility_policy(rhs);
//      break;
//    case msgpack_attr_rename_policy:
//      kphp_error_act(!rhs.empty(), "@kphp-msgpack 'rename_policy' expected to have a value after '='", break);
//      msgpack_tag.rename_policy = parse_rename_policy(rhs);
//      break;
//    case msgpack_attr_flatten:
//      msgpack_tag.flatten = parse_bool_or_true_if_nothing(attr_type, rhs);
//      break;
//    case msgpack_attr_fields:
//      kphp_error_act(!rhs.empty(), "@kphp-msgpack 'fields' expected to have a value after '='", break);
//      msgpack_tag.fields = parse_fields_delimited_by_comma(rhs);
//      break;
    case msgpack_attr_unknown:
    default:
      kphp_error(0, fmt_format("Unknown @kphp-msgpack attribute '{}'", attr_name));
      break;
  }
  return msgpack_tag;
}


void KphpMsgPackTagList::add_tag(const KphpMsgPackTag &msgpack_tag) {
  // when adding, check for duplicates
  // if both 'for' and no-'for' for the same attr exist, 'for' must be placed below: it allows linear pattern matching
  for (const KphpMsgPackTag &existing : tags) {
    if (existing.attr_type == msgpack_tag.attr_type) {
      if (existing.for_encoder == msgpack_tag.for_encoder) {
        kphp_error(existing.for_encoder, fmt_format("@kphp-msgpack '{}' is duplicated", AllMsgPackAttrs::type2name(msgpack_tag.attr_type)));
        kphp_error(!existing.for_encoder, fmt_format("@kphp-msgpack for {} '{}' is duplicated", existing.for_encoder->as_human_readable(), AllMsgPackAttrs::type2name(msgpack_tag.attr_type)));
      } else if (existing.for_encoder) {
        kphp_error(msgpack_tag.for_encoder, fmt_format("@kphp-msgpack for {} '{}' should be placed below @kphp-msgpack '{}' without for", existing.for_encoder->as_human_readable(), AllMsgPackAttrs::type2name(msgpack_tag.attr_type), AllMsgPackAttrs::type2name(msgpack_tag.attr_type)));
      }
    }
  }
  
  tags.emplace_back(msgpack_tag);
}

void KphpMsgPackTagList::validate_tags_compatibility([[gnu::unused]] ClassPtr above_class) const {
//  unsigned int existing_mask = 0;
//  for (const KphpMsgPackTag &tag : tags) {
//    existing_mask |= tag.attr_type;
//  }

//  for (const KphpMsgPackTag &tag : tags) {
//    switch (tag.attr_type) {
//      case msgpack_attr_flatten:
//        if (tag.flatten) {
//          constexpr unsigned int disallowed_over_class = msgpack_attr_rename_policy | msgpack_attr_visibility_policy | msgpack_attr_skip_if_default | msgpack_attr_fields;
//          kphp_error((disallowed_over_class & existing_mask) == 0, "@kphp-msgpack 'flatten' can't be used with some other @kphp-msgpack tags");
//          kphp_error(!tag.for_encoder, "@kphp-msgpack 'flatten' can't be used with 'for', it's a global state");
//        }
//        break;
//      case msgpack_attr_fields:
//        if (above_class) {
//          for (const auto &field_name : split_skipping_delimeters(tag.fields, ",")) {
//            kphp_error(above_class->members.has_field(field_name), fmt_format("@kphp-msgpack 'fields' specifies '{}', but such field doesn't exist in class {}", field_name, above_class->as_human_readable()));
//          }
//        }
//      default:
//        break;
//    }
//  }
}

//void KphpMsgPackTagList::check_flatten_class(ClassPtr flatten_class) const {
//  int members_count = 0;
//  flatten_class->members.for_each([&members_count](const ClassMemberInstanceField &f) {
//    members_count++;
//
//    if (f.kphp_msgpack_tags) {
//      constexpr unsigned int disallowed_over_field = msgpack_attr_rename | msgpack_attr_skip | msgpack_attr_required | msgpack_attr_skip_if_default;
//      for (const KphpMsgPackTag &f_tag : *f.kphp_msgpack_tags) {
//        kphp_error((disallowed_over_field & f_tag.attr_type) == 0,
//                   fmt_format("@kphp-msgpack '{}' over a field is disallowed for flatten classes", AllMsgPackAttrs::type2name(f_tag.attr_type)));
//      }
//    }
//  });
//
//  kphp_error(members_count == 1,
//             "@kphp-msgpack 'flatten' class must have exactly one field");
//  kphp_error(!flatten_class->parent_class && flatten_class->derived_classes.empty(),
//             "@kphp-msgpack 'flatten' class can not extend anything or have derived classes");
//}

const KphpMsgPackTagList *KphpMsgPackTagList::create_from_phpdoc(FunctionPtr resolve_context, const PhpDocComment *phpdoc, ClassPtr above_class) {
  auto *list = new KphpMsgPackTagList;

  for (const PhpDocTag &tag : phpdoc->tags) {
    if (tag.type == PhpDocType::kphp_msgpack) {
      KphpMsgPackTag msgpack_tag = parse_kphp_msgpack_tag(resolve_context, vk::trim(tag.value));
      if (msgpack_tag.attr_type == msgpack_attr_unknown) {
        continue;
      }

      if (above_class) {
        kphp_error(msgpack_tag.attr_type & ATTRS_ALLOWED_FOR_CLASS,
                   fmt_format("@kphp-msgpack '{}' is allowed above fields, not above classes", AllMsgPackAttrs::type2name(msgpack_tag.attr_type)));
      } else {
        kphp_error(msgpack_tag.attr_type & ATTRS_ALLOWED_FOR_FIELD,
                   fmt_format("@kphp-msgpack '{}' is allowed above classes, not above fields", AllMsgPackAttrs::type2name(msgpack_tag.attr_type)));
      }
      list->add_tag(msgpack_tag);
    }
  }

  list->validate_tags_compatibility(above_class);
  return list;
}


//std::string transform_msgpack_name_to(RenamePolicy policy, vk::string_view name) noexcept {
//  switch (policy) {
//    case RenamePolicy::snake_case:
//      return transform_to_snake_case(name);
//    case RenamePolicy::camel_case:
//      return transform_to_camel_case(name);
//    default:
//      return std::string{name};
//  }
//}

// a PHP class MsgPackEncoder (and its custom inheritors) have `const visibility_policy` and others
// their values have exactly the same format as could be written in @kphp-msgpack above jsonable classes
// this function converts constants (all of them surely exist) into the same structure
//const KphpMsgPackTagList *convert_encoder_constants_to_tags(ClassPtr msgpack_encoder) {
//  auto *contants_as_tags = new KphpMsgPackTagList;
//  KphpMsgPackTag from_const{.attr_type = msgpack_attr_unknown, .for_encoder = ClassPtr{}};
//  std::string s_const;
//  VertexPtr v_const;

//  v_const = msgpack_encoder->get_constant("skip_if_default")->value;
//  s_const = v_const->type() == op_false ? "false" : v_const->type() == op_true ? "true" : "`const skip_if_default` not bool";
//  from_const.attr_type = msgpack_attr_skip_if_default;
//  from_const.skip_if_default = parse_bool_or_true_if_nothing(msgpack_attr_skip_if_default, s_const);
//  contants_as_tags->add_tag(from_const);
//
//  v_const = msgpack_encoder->get_constant("rename_policy")->value;
//  s_const = v_const->type() == op_string ? v_const->get_string() : "`const rename_policy` not string";
//  from_const.attr_type = msgpack_attr_rename_policy;
//  from_const.rename_policy = parse_rename_policy(s_const);
//  contants_as_tags->add_tag(from_const);
//
//  v_const = msgpack_encoder->get_constant("visibility_policy")->value;
//  s_const = v_const->type() == op_string ? v_const->get_string() : "`const visibility_policy` not string";
//  from_const.attr_type = msgpack_attr_visibility_policy;
//  from_const.visibility_policy = parse_visibility_policy(s_const);
//  contants_as_tags->add_tag(from_const);
//
//  v_const = msgpack_encoder->get_constant("float_precision")->value;
//  s_const = v_const->type() == op_int_const ? v_const->get_string() : "`const float_precision` not int";
//  from_const.attr_type = msgpack_attr_float_precision;
//  from_const.float_precision = parse_float_precision(s_const);
//  contants_as_tags->add_tag(from_const);

//  return contants_as_tags;
//}

FieldMsgPackSettings merge_and_inherit_msgpack_tags(const ClassMemberInstanceField &field, ClassPtr klass, ClassPtr msgpack_encoder) {
  (void)field;
  (void)klass;
  (void)msgpack_encoder;
  FieldMsgPackSettings s;

//
//  // for 'public int $id;' — no default and non-nullable type — set 'required' so that decode() would fire unless exists
//  // it could be overridden with `@kphp-msgpack required = false`
//  if (field.type_hint && !field.var->init_val && !does_type_hint_allow_null(field.type_hint)) {
//    s.required = true;
//  }
//
//  // loop over constants in encoder that can be overridden by a class
//  for (const KphpMsgPackTag &encoder_const : *msgpack_encoder->kphp_msgpack_tags) {
//    const KphpMsgPackTag *override_klass_tag{nullptr};
//    if (klass->kphp_msgpack_tags) {
//      override_klass_tag = klass->kphp_msgpack_tags->find_tag([encoder_const, msgpack_encoder](const KphpMsgPackTag &tag){
//        return tag.attr_type == encoder_const.attr_type && (!tag.for_encoder || tag.for_encoder == msgpack_encoder);
//      });
//    }
//    const KphpMsgPackTag *tag = override_klass_tag ?: &encoder_const;
//
//    switch (tag->attr_type) {
//      case msgpack_attr_rename_policy:
//        s.msgpack_key = transform_msgpack_name_to(tag->rename_policy, field.local_name());
//        break;
//      case msgpack_attr_skip_if_default:
//        s.skip_if_default = tag->skip_if_default;
//        break;
//      case msgpack_attr_float_precision:
//        s.float_precision = tag->float_precision;
//        break;
//      case msgpack_attr_visibility_policy: {
//        bool skip_unless_changed_below = tag->visibility_policy == VisibilityPolicy::all ? false : !field.modifiers.is_public();
//        s.skip_when_encoding = skip_unless_changed_below;
//        s.skip_when_decoding = skip_unless_changed_below;
//        break;
//      }
//      default:
//        kphp_assert(0 && "unexpected json attr_type in class/encoder codegen");
//    }
//  }
//
//  // loop over attrs that can be only above class
//  if (klass->kphp_msgpack_tags) {
//    for (const KphpMsgPackTag &klass_tag : *klass->kphp_msgpack_tags) {
//      if (klass_tag.for_encoder && klass_tag.for_encoder != msgpack_encoder) {
//        continue;
//      }
//
//      switch (klass_tag.attr_type) {
//        case msgpack_attr_flatten:
//          s.skip_if_default = false;
//          s.skip_when_encoding = false;
//          s.skip_when_decoding = false;
//          break;
//        case msgpack_attr_fields: {
//          bool skip_unless_changed_below = klass_tag.fields.find("," + field.local_name() + ",") == std::string::npos;
//          s.skip_when_encoding = skip_unless_changed_below;
//          s.skip_when_decoding = skip_unless_changed_below;
//          break;
//        }
//        default:
//          break;
//      }
//    }
//  }
//
//  // loop over attrs above field: they override class/encoder
//  if (field.kphp_msgpack_tags) {
//    for (const KphpMsgPackTag &tag : *field.kphp_msgpack_tags) {
//      if (tag.for_encoder && tag.for_encoder != msgpack_encoder) {
//        continue;
//      }
//
//      switch (tag.attr_type) {
//        case msgpack_attr_rename:
//          s.msgpack_key = static_cast<std::string>(tag.rename);
//          break;
//        case msgpack_attr_skip:
//          s.skip_when_decoding = tag.skip == SkipFieldType::skip_decode_only || tag.skip == SkipFieldType::always_skip;
//          s.skip_when_encoding = tag.skip == SkipFieldType::skip_encode_only || tag.skip == SkipFieldType::always_skip;
//          break;
//        case msgpack_attr_array_as_hashmap:
//          s.array_as_hashmap = tag.array_as_hashmap;
//          break;
//        case msgpack_attr_raw_string:
//          s.raw_string = tag.raw_string;
//          break;
//        case msgpack_attr_required:
//          s.required = tag.required;
//          break;
//        case msgpack_attr_float_precision:
//          s.float_precision = tag.float_precision;
//          break;
//        case msgpack_attr_skip_if_default:
//          s.skip_if_default = tag.skip_if_default;
//          break;
//        default:
//          kphp_assert(0 && "unexpected json attr_type in field codegen");
//      }
//    }
//  }
//
  return s;
}

} // namespace kphp_json
