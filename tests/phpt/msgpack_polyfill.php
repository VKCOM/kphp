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

  public abstract function from_unpacked_value($value, UseResolver $use_resolver);

  public abstract function has_instance_inside(): bool;
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

  public function from_unpacked_value($value, UseResolver $use_resolver) {
    try {
      return $this->type1->from_unpacked_value($value, $use_resolver);
    } catch (Throwable $e) {
      return $this->type2->from_unpacked_value($value, $use_resolver);
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

  public function from_unpacked_value($value, UseResolver $use_resolver) {
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

  public function from_unpacked_value($value, UseResolver $use_resolver) {
    $resolved_class_name = $use_resolver->resolve_name($this->type);
    if (!class_exists($resolved_class_name)) {
      throw new Exception("Can't find class: {$resolved_class_name}");
    }
    $parser = new InstanceParser($resolved_class_name);
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

  public function from_unpacked_value($arr, UseResolver $use_resolver, ?int $cnt_arrays = null) {
    if (!is_array($arr)) {
      throw new Exception("not instance: ".gettype($this->inner_type));
    }

    if (!$this->has_instance_inside()) {
      return $arr;
    }

    if ($cnt_arrays === null) {
      return $this->from_unpacked_value($arr, $use_resolver, $this->cnt_arrays);
    }

    $res = [];
    foreach ($arr as $value) {
      $res[] = $cnt_arrays === 1
        ? $this->inner_type->from_unpacked_value($value, $use_resolver)
        : $this->from_unpacked_value($value, $use_resolver, $cnt_arrays - 1);
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

  public function from_unpacked_value($value, UseResolver $use_resolver) {
    if (!is_array($value)) {
      throw new Exception("not tuple: ".implode(", ", $this->types));
    }

    if (count($this->types) != count($value)) {
      throw new Exception("different counts: ".count($this->types)." and ".count($value));
    }

    $res = [];
    for ($i = 0; $i < count($value); $i++) {
      $res[] = $this->types[$i]->from_unpacked_value($value[$i], $use_resolver);
    }

    return $res;
  }

  public function has_instance_inside(): bool {
    return in_array(true, array_map([$this, "has_instance_inside"], $this->types));
  }
}

class InstanceParser
{
  /**@var mixed[]*/
  public $tags_values = [];

  /**@var string[]*/
  public $names = [];

  /**@var string[]*/
  public $types = [];

  /**@var ReflectionClass*/
  public $type_of_instance = null;

  /**@var UseResolver*/
  public $use_resolver = null;

  public function __construct($instance) {
    assert(is_object($instance) || ($instance !== "" && $instance !== "self"));
    $this->type_of_instance = new \ReflectionClass($instance);
    $this->use_resolver = new UseResolver($this->type_of_instance->getName());

    foreach ($this->type_of_instance->getProperties() as $property) {
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

    $instance = $this->type_of_instance->newInstanceWithoutConstructor();

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
      $instance->{$cur_name} = $parsed_phpdoc->from_unpacked_value($cur_value, $this->use_resolver);
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

class UseResolver {
  /**@var ReflectionClass*/
  private $instance_reflection = null;

  /**@var string[]*/
  private $alias_to_name = [];

  /**@var string*/
  private $cur_namespace = "";

  /**@var mixed[]*/
  private $tokens = [];

  /**@var int*/
  private $token_id = -1;

  public function __construct(string $instance_type) {
    $this->instance_reflection = new \ReflectionClass($instance_type);

    $content = '';
    $file = new \SplFileObject($this->instance_reflection->getFileName());
    for ($line_id = 0; !$file->eof() && $line_id < $this->instance_reflection->getStartLine(); $line_id++) {
      $content .= $file->fgets();
    }

    $this->find_uses($content);
  }

  public function resolve_name(string $instance_name) {
    if ($instance_name[0] === '\\') {
      return $instance_name;
    }

    if ($instance_name === 'self') {
      return $this->instance_reflection->getName();
    }

    $backslash_position = strstr($instance_name, '\\') ?: strlen($instance_name);
    $first_part_of_name = substr($instance_name, 0, $backslash_position);
    if (isset($this->alias_to_name[$first_part_of_name])) {
      return $this->alias_to_name[$first_part_of_name].substr($instance_name, strlen($first_part_of_name));
    }

    return $this->instance_reflection->getNamespaceName().'\\'.$instance_name;
  }

  private function cur_token() {
    return $this->tokens[$this->token_id];
  }

  private function is_token(array $tokens) {
    return in_array($this->cur_token()[0], $tokens, true);
  }

  private function get_next_token() {
    while ($this->token_id + 1 < count($this->tokens)) {
      $this->token_id++;
      if (!$this->is_token([T_WHITESPACE, T_COMMENT, T_DOC_COMMENT])) {
        return $this->cur_token();
      }
    }
    return null;
  }

  private function parse_class_name() : array {
    $class_name = "";
    $alias = "";
    while ($this->get_next_token() && $this->is_token([T_STRING, T_NS_SEPARATOR])) {
      $class_name .= $this->cur_token()[1];
      $alias = $this->cur_token()[1];
    }
    return [$alias, $class_name];
  }


  private function parse_use_statement() : ?array {
    if ($this->cur_token() === ';' || $this->cur_token() === null) {
      return null;
    }

    [$alias, $class_name] = $this->parse_class_name();
    if ($this->is_token([T_AS])) {
      [,$alias] = $this->parse_class_name();
    }

    return [$alias, $class_name];
  }

  private function parse_namespace() {
    [,$this->cur_namespace] = $this->parse_class_name();
  }

  private function find_uses(string $file_content) {
    $this->tokens = token_get_all($file_content);

    /**
     * use My\Full\Classname as Another;
     * use My\Full\Classname as Another, My\Full\NSname;
     * use My\Full\NSname;
     * use \My\Fulle\Name;
     */
    while ($this->get_next_token()) {
      if ($this->is_token([T_NAMESPACE])) {
        $this->parse_namespace();
      } else if ($this->is_token([T_USE])) {
        while (list($alias, $class) = $this->parse_use_statement()) {
          $this->alias_to_name[$alias] = $class;
        }
      }
    }
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
