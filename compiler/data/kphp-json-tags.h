// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <vector>

#include "common/wrappers/string_view.h"
#include "compiler/data/data_ptr.h"

struct ClassMemberInstanceField;

namespace kphp_json {

enum class RenamePolicy { none, snake_case, camel_case };

enum class VisibilityPolicy { all, public_only };

enum class SkipFieldType {
  dont_skip,
  always_skip,
  skip_encode_only,
  skip_decode_only,
};

enum JsonAttrType {
  json_attr_unknown = 0,
  json_attr_rename = 1 << 1,
  json_attr_skip = 1 << 2,
  json_attr_array_as_hashmap = 1 << 3,
  json_attr_raw_string = 1 << 4,
  json_attr_required = 1 << 5,
  json_attr_float_precision = 1 << 6,
  json_attr_skip_if_default = 1 << 7,
  json_attr_visibility_policy = 1 << 8,
  json_attr_rename_policy = 1 << 9,
  json_attr_flatten = 1 << 10,
  json_attr_fields = 1 << 11,
};

// one `@kphp-json attr=value` is represented as this class
// depending on attr_type, one of union fields is set
// there could be for statement: `@kphp-json for MyEncoder attr=value`
struct KphpJsonTag {
  JsonAttrType attr_type{json_attr_unknown};
  ClassPtr for_encoder;

  union {
    vk::string_view rename;
    SkipFieldType skip;
    bool array_as_hashmap;
    bool raw_string;
    bool required;
    int float_precision{0};
    bool skip_if_default;
    VisibilityPolicy visibility_policy;
    RenamePolicy rename_policy;
    bool flatten;
    vk::string_view fields;
  };
};

// if a class/field has at least one @kphp-json above,
// this class containing all such tags from phpdoc is created
class KphpJsonTagList {
  std::vector<KphpJsonTag> tags;

public:
  bool empty() const noexcept {
    return tags.empty();
  }
  auto begin() const noexcept {
    return tags.begin();
  }
  auto end() const noexcept {
    return tags.end();
  }

  template<class CallbackT>
  const KphpJsonTag *find_tag(const CallbackT &callback) const noexcept {
    // no-'for' tag appears above 'for', so find the last one satisfying
    const KphpJsonTag *last{nullptr};
    for (const KphpJsonTag &tag : tags) {
      if (callback(tag)) {
        last = &tag;
      }
    }
    return last;
  }

  void add_tag(const KphpJsonTag &json_tag);
  void validate_tags_compatibility(ClassPtr above_class) const;
  void check_flatten_class(ClassPtr flatten_class) const;

  static const KphpJsonTagList *create_from_phpdoc(FunctionPtr resolve_context, const PhpDocComment *phpdoc, ClassPtr above_class);
};

// represents final settings for a field
// after merging its @kphp-json tags with @kphp-json of a class and encoder constants at exact call
struct FieldJsonSettings {
  std::string json_key;
  bool skip_when_encoding{false};
  bool skip_when_decoding{false};
  bool skip_if_default{false};
  bool array_as_hashmap{false};
  bool required{false};
  bool raw_string{false};
  int float_precision{0};
};

std::string transform_json_name_to(RenamePolicy policy, vk::string_view name) noexcept;
const KphpJsonTagList *convert_encoder_constants_to_tags(ClassPtr json_encoder);
FieldJsonSettings merge_and_inherit_json_tags(const ClassMemberInstanceField &field, ClassPtr klass, ClassPtr json_encoder);

} // namespace kphp_json
