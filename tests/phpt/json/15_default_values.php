@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class A {
  public string $foo = "default_foo";
  public string $bar = "default_bar";
}

function test_default_first_field() {
  $json = "{\"bar\":\"baz\"}";
  $obj = JsonEncoder::decode($json, "A");
  var_dump(to_array_debug($obj));
}

function test_default_second_field() {
  $json = "{\"foo\":\"baz\"}";
  $obj = JsonEncoder::decode($json, "A");
  var_dump(to_array_debug($obj));
}

function test_default_both_fields() {
  $json = "{}";
  $obj = JsonEncoder::decode($json, "A");
  var_dump(to_array_debug($obj));
}

class B {
  /** @var string */
  public $phpdoc_string_uninited;
  /** @var string */
  public $phpdoc_string_inited_default = '';
    /** @var string */
  public $phpdoc_string_inited = 'value';

  /** @var ?string */
  public $optional_phpdoc_string_uninited;
  /** @var ?string */
  public $optional_phpdoc_string_inited_default = '';
    /** @var ?string */
  public $optional_phpdoc_string_inited = 'value';

  public string $string_uninited;
  public string $string_inited_default = '';
  public string $string_inited = 'value';

  public ?string $optional_string_uninited;
  public ?string $optional_string_inited_default = '';
  public ?string $optional_string_inited = 'value';
}

function test_phpdoc_defaults() {
  $json = "{}";
  $obj = JsonEncoder::decode($json, B::class);
  var_dump(to_array_debug($obj));
}

test_default_first_field();
test_default_second_field();
test_default_both_fields();
test_phpdoc_defaults();
