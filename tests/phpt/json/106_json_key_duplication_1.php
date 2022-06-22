@kphp_should_fail
/Json key "foo_bar_baz" appears twice for class A encoded with JsonEncoder/
/Json key "fooBarBaz" appears twice for class Foo encoded with JsonEncoder/
<?php

/** @kphp-json rename_policy=camelCase */
class Foo {
  public string $foo_bar_baz = "value";
  /** @kphp-json rename=fooBarBaz */
  public float $qux_qax_quz = 12.34;
}

class C {
  public string $foo_bar_baz = "c_value";
}

class B extends C {
  public string $b = "b_value";
}

/** @kphp-json rename_policy=snake_case */
class A extends B {
  public string $fooBarBaz = "a_value";
}

function test_key_duplication() {
  $json = JsonEncoder::encode(new Foo);
}

function test_key_duplication_base_class() {
  $json = JsonEncoder::encode(new A);
}

test_key_duplication();
test_key_duplication_base_class();
