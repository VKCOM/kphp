// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <vector>

#include "compiler/data/data_ptr.h"
#include "common/wrappers/string_view.h"

struct ClassMemberInstanceField;

namespace kphp_msgpack {

//enum class RenamePolicy {
//  none,
//  snake_case,
//  camel_case
//};

//enum class VisibilityPolicy {
//  all,
//  public_only
//};

enum class SkipFieldType {
  dont_skip,
  always_skip,
  skip_encode_only,
  skip_decode_only,
};

enum MsgPackAttrType {
  msgpack_attr_unknown = 0,
  msgpack_attr_key = 1 << 1,
  msgpack_attr_array_as_hashmap = 1 << 2,
//  msgpack_attr_fields = 1 << 3,
//  msgpack_attr_rename = 1 << 1,
  msgpack_attr_skip = 1 << 3,
  msgpack_attr_use_keys = 1 << 4,
  msgpack_attr_use_names = 1 << 5,
//  msgpack_attr_required = 1 << 5,
//  msgpack_attr_float_precision = 1 << 6,
//  msgpack_attr_skip_if_default = 1 << 7,
//  msgpack_attr_visibility_policy = 1 << 8,
//  msgpack_attr_rename_policy = 1 << 9,
//  msgpack_attr_flatten = 1 << 10,
};


// one `@kphp-msgpack attr=value` is represented as this class
// depending on attr_type, one of union fields is set
// there could be for statement: `@kphp-msgpack for MyEncoder attr=value`
struct KphpMsgPackTag {
  MsgPackAttrType attr_type{msgpack_attr_key};
  ClassPtr for_encoder;
  
  union { 
//    vk::string_view rename;
    SkipFieldType skip;
    bool use_keys;
    bool use_names;
    bool array_as_hashmap;
    int key;
//    bool raw_string;
//    bool required;
//    int float_precision{0};
//    bool skip_if_default;
//    VisibilityPolicy visibility_policy;
//    RenamePolicy rename_policy;
//    bool flatten;
//    vk::string_view fields;
  };
};

// if a class/field has at least one @kphp-msgpack above,
// this class containing all such tags from phpdoc is created
class KphpMsgPackTagList {
  std::vector<KphpMsgPackTag> tags;

public:
  bool empty() const noexcept { return tags.empty(); }
  auto begin() const noexcept { return tags.begin(); }
  auto end() const noexcept { return tags.end(); }

  template<class CallbackT>
  const KphpMsgPackTag *find_tag(const CallbackT &callback) const noexcept {
    // no-'for' tag appears above 'for', so find the last one satisfying
    const KphpMsgPackTag *last{nullptr};
    for (const KphpMsgPackTag &tag : tags) {
      if (callback(tag)) {
        last = &tag;
      }
    }
    return last;
  }

  void add_tag(const KphpMsgPackTag &json_tag);
  void validate_tags_compatibility(ClassPtr above_class) const;
  void check_flatten_class(ClassPtr flatten_class) const;

  static const KphpMsgPackTagList *create_from_phpdoc(FunctionPtr resolve_context, const PhpDocComment *phpdoc, ClassPtr above_class);
};

// represents final settings for a field
// after merging its @kphp-msgpack tags with @kphp-msgpack of a class and encoder constants at exact call
struct FieldMsgPackSettings {
  int msgpack_key{-1};
//  std::string json_key;
//  bool skip_when_encoding{false};
//  bool skip_when_decoding{false};
//  bool skip_if_default{false};
  bool array_as_hashmap{false};
//  bool required{false};
//  bool raw_string{false};
//  int float_precision{0};
};

//std::string transform_json_name_to(RenamePolicy policy, vk::string_view name) noexcept;
const KphpMsgPackTagList *convert_encoder_constants_to_tags(ClassPtr msgpack_encoder);
FieldMsgPackSettings merge_and_inherit_msgpack_tags(const ClassMemberInstanceField &field, ClassPtr klass, ClassPtr msgpack_encoder);

} // namespace kphp_json
