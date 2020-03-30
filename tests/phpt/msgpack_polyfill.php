<?php

use MessagePack\MessagePack;
use MessagePack\Packer;
use MessagePack\TypeTransformer\CanPack;

abstract class PHPDocType {
  /*
    PHPDoc grammar

    # according to https://www.php.net/manual/en/language.oop5.basic.php
    InstanceType  ::= ^\?[A-Z][a-zA-Z0-9_\x80-\xff\]* | "self" | "static" | "object"

    #according to https://docs.phpdoc.org/latest/guides/types.html
    PrimitiveType ::= "int"     | "integer" | "float" | "string" | "array" | "mixed"
                      "boolean" | "bool"    | "false" | "true"   | "null"  | "NULL"

    TupleType     ::= "\"? "tuple(" PHPDocType ("," PHPDocType)* ")"
    ArrayType     ::= ("(" PHPDocType ")" | PHPDocType) "[]"*
    OrType        ::= PHPDocType "|" PHPDocType
    PHPDocType    ::= InstanceType  |
                      PrimitiveType |
                      TupleType     |
                      ArrayType     |
                      OrType        |
  */
  protected static function parse_impl(string &$str): ?PHPDocType {
    $res = InstanceType::parse($str) ?:
           PrimitiveType::parse($str) ?:
           TupleType::parse($str) ?:
           ArrayType::parse($str);

    if (!$res) {
      return null;
    }

    if ($cnt_arrays = ArrayType::parse_arrays($str)) {
      $res = new ArrayType($res, $cnt_arrays);
    }

    if ($or_type = OrType::parse($str)) {
      $or_type->type1 = $res;
      $res = $or_type;
    }

    return $res;
  }

  public static function parse(string &$str) : ?PHPDocType {
    $str = ltrim($str);
    $res = static::parse_impl($str);
    $str = ltrim($str);
    return $res;
  }

  public static function remove_if_starts_with(string &$haystack, $needle): bool {
    if (strpos($haystack, $needle) === 0) {
      $haystack = substr($haystack, strlen($needle));
      return true;
    }
    return false;
  }

  public abstract function from_unpacked_value($value);

  public abstract function has_instance_inside(): bool;

  public static $outer_class_name_for_resolving_self;
}

class OrType extends PHPDocType {
  /**@var PHPDocType */
  public $type1;

  /**@var PHPDocType */
  public $type2;

  protected static function parse_impl(string &$str): ?PHPDocType {
    if (parent::remove_if_starts_with($str, '|')) {
      $res = new OrType();
      $res->type2 = PHPDocType::parse($str);
      return $res;
    }

    return null;
  }

  public function from_unpacked_value($value) {
    try {
      return $this->type1->from_unpacked_value($value);
    } catch (Throwable $e) {
      return $this->type2->from_unpacked_value($value);
    }
  }

  public function has_instance_inside(): bool {
    return $this->type1->has_instance_inside() || $this->type2->has_instance_inside();
  }
}

class PrimitiveType extends PHPDocType {
  /**@var string*/
  public $type = "";
  public static $primitive_types = [
    "int", "integer", "float",
    "string", "array",
    "boolean", "bool", "false", "true",
    "null", "NULL",
    "mixed",
  ];

  protected static function parse_impl(string &$str): ?PHPDocType {
    foreach (PrimitiveType::$primitive_types as $primitive_type) {
      if (self::remove_if_starts_with($str, $primitive_type)) {
        $res = new PrimitiveType();
        $res->type = $primitive_type;
        return $res;
      }
    }

    return null;
  }

  public function from_unpacked_value($value) {
    $true_type = $this->type;
    switch ($this->type) {
      case "int":
        $true_type = "integer";
        break;
      case "float":
        $true_type = "double";
        break;
      case "boolean":
      case "bool":
      case "false":
      case "true":
        $true_type = "boolean";
        break;
      case "null":
        $true_type = "NULL";
        break;
    }

    if (gettype($value) == $true_type || $true_type == "mixed") {
      return $value;
    }

    throw new Exception("not primitive: ".$this->type);
  }

  public function has_instance_inside(): bool {
    return false;
  }
}

class InstanceType extends PHPDocType {
  /**@var string*/
  public $type = "";

  protected static function parse_impl(string &$str): ?PHPDocType {

    if (preg_match("/^(\\\\?[A-Z][a-zA-Z0-9_\x80-\xff\\\\]*|self|static|object)/", $str, $matches)) {
      $res = new InstanceType();
      $res->type = $matches[1];
      $str = substr($str, strlen($res->type));

      if (in_array($res->type, ["static", "object"])) {
        throw new Exception("static|object are forbidden in phpdoc");
      }
      return $res;
    }

    return null;
  }

  public function from_unpacked_value($value) {
    $parser = new InstanceParser($this->type);
    return $parser->from_unpacked_array($value);
  }

  public function has_instance_inside(): bool {
    return true;
  }
}

class ArrayType extends PHPDocType {
  /**@var PHPDocType*/
  public $inner_type = null;

  /**@var int*/
  public $cnt_arrays = 0;

  public function __construct(PHPDocType $inner_type, int $cnt_arrays) {
    $this->inner_type = $inner_type;
    $this->cnt_arrays = $cnt_arrays;
  }

  public static function parse_arrays(string &$str): int {
    $cnt_arrays = 0;
    while (parent::remove_if_starts_with($str, "[]")) {
      $cnt_arrays += 1;
    }

    return $cnt_arrays;
  }

  protected static function parse_impl(string &$str): ?PHPDocType {
    if (parent::remove_if_starts_with($str, '(')) {
      $inner_type = PHPDocType::parse($str);
      assert($inner_type && $str[0] == ')');
      $str = ltrim(substr($str, 1));
    } else {
      return null;
    }

    if ($cnt_arrays = self::parse_arrays($str)) {
      return new ArrayType($inner_type, $cnt_arrays);
    }

    return $inner_type;
  }

  public function from_unpacked_value($arr, ?int $cnt_arrays = null) {
    if (!is_array($arr)) {
      throw new Exception("not instance: ".gettype($this->inner_type));
    }

    if (!$this->has_instance_inside()) {
      return $arr;
    }

    if ($cnt_arrays === null) {
      return $this->from_unpacked_value($arr, $this->cnt_arrays);
    }

    $res = [];
    foreach ($arr as $value) {
      $res[] = $cnt_arrays === 1
        ? $this->inner_type->from_unpacked_value($value)
        : $this->from_unpacked_value($value, $cnt_arrays - 1);
    }

    return $res;
  }

  public function has_instance_inside(): bool {
    return $this->inner_type->has_instance_inside();
  }
}

class TupleType extends PHPDocType {
  /**@var PHPDocType[]*/
  public $types = [];

  /** @param $types PHPDocType[] */
  public function __construct($types) {
    $this->types = $types;
  }

  protected static function parse_impl(string &$str): ?PHPDocType {
    if (!parent::remove_if_starts_with($str, "\\tuple(") && !parent::remove_if_starts_with($str, "tuple(")) {
      return null;
    }

    $types = [];
    while (true) {
      $cur_type = PHPDocType::parse($str);
      if (!$cur_type) {
        throw new Exception("something went wrong in parsing tuple phpdoc");
      }

      $types[] = $cur_type;
      $str = ltrim($str);
      if ($str[0] == ',') {
        $str = substr($str, 1);
      } else if ($str[0] == ')') {
        $str = substr($str, 1);
        break;
      } else {
        throw new Exception("phpdoc parsing error ',' or ')' expected");
      }
    }

    return new TupleType($types);
  }

  public function from_unpacked_value($value) {
    if (!is_array($value)) {
      throw new Exception("not tuple: ".implode(", ", $this->types));
    }

    if (count($this->types) != count($value)) {
      throw new Exception("different counts: ".count($this->types)." and ".count($value));
    }

    $res = [];
    for ($i = 0; $i < count($value); $i++) {
      $res[] = $this->types[$i]->from_unpacked_value($value[$i]);
    }

    return $res;
  }

  public function has_instance_inside(): bool {
    return in_array(true, array_map([$this, "has_instance_inside"], $this->types));
  }
}

class InstanceParser
{
  public $tags_values = [];
  public $names = [];
  public $types = [];
  public $type_of_instance = null;

  public function __construct($instance) {
    if ($instance == "self") {
      $instance = PHPDocType::$outer_class_name_for_resolving_self;
    } else {
      PHPDocType::$outer_class_name_for_resolving_self = $instance;
    }
    assert(!empty($instance) && $instance != "self");

    $this->type_of_instance = $instance;
    $rc = new ReflectionClass($instance);

    foreach ($rc->getProperties() as $property) {
      preg_match("/@kphp-serialized-field\s+(\d+)/", $property->getDocComment(), $matches);
      if (count($matches) > 1) {
        $this->tags_values[] = (int) $matches[1];
        $this->tags_values[] = is_object($instance) ? $property->getValue($instance) : null;

        preg_match("/@var\s+([^\s]+)/", $property->getDocComment(), $matches);
        assert(count($matches) > 1);

        $this->types[] = $matches[1];
        $this->names[] = $property->getName();
      }
    }
  }

  public function from_unpacked_array($unpacked_arr) {
    if (is_null($unpacked_arr)) {
      return null;
    } else if (!is_array($unpacked_arr)) {
      throw new Exception("Expected NIL or ARRAY type for unpacking class_instance");
    }

    $instance = new $this->type_of_instance;

    $is_even = function ($key) { return $key % 2 == 0; };
    $tags = array_values(array_filter($this->tags_values, $is_even, ARRAY_FILTER_USE_KEY));

    for ($i = 0; $i < count($unpacked_arr); $i += 2) {
      $cur_tag = $unpacked_arr[$i];
      $cur_value = $unpacked_arr[$i + 1];

      $cur_idx = array_search($cur_tag, $tags);
      $cur_type = $this->types[$cur_idx];
      $cur_name = $this->names[$cur_idx];

      $parsed_phpdoc = PHPDocType::parse($cur_type);
//      print_r($parsed_phpdoc);
      $instance->{$cur_name} = $parsed_phpdoc->from_unpacked_value($cur_value);
    }

    return $instance;
  }
}

class ClassTransformer implements CanPack {
  /** @var int */
  public static $depth = 0;

  /** @var int */
  public static $max_depth = 20;

  public function pack(Packer $packer, $instance): ?string
  {
    ClassTransformer::$depth++;
    if (ClassTransformer::$depth > ClassTransformer::$max_depth) {
      throw new Exception("maximum depth of nested instances exceeded");
    }

    $instance_parser = new InstanceParser($instance);
    $res = $packer->pack($instance_parser->tags_values);
    ClassTransformer::$depth--;

    return $res;
  }
}

function instance_serialize($instance) {
  ClassTransformer::$depth = 0;
  $packer = new Packer();
  $packer = $packer->extendWith(new ClassTransformer());
  return $packer->pack($instance);
}

function instance_deserialize($packed_str, $type_of_instance) {
  $unpacked_array = MessagePack::unpack($packed_str);

  $instance_parser = new InstanceParser($type_of_instance);
  return $instance_parser->from_unpacked_array($unpacked_array);
}

function msgpack_serialize($value) {
  $packer = new Packer();
  return $packer->pack($value);
}

function msgpack_deserialize($packed_str) {
  return MessagePack::unpack($packed_str);
}
