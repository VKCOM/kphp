@kphp_should_fail
/Field B::\$b is @kphp-json 'array_as_hashmap', but it's not an array/
/Field D::\$d is @kphp-json 'array_as_hashmap', but it's not an array/
<?php

class B {
  /** @kphp-json array_as_hashmap */
  public string $b = "a_value";
}

class A extends B {
  public string $a = "a_value";
}

function test_incorrect_tags() {
  JsonEncoder::encode(new A);
}

class D {
  /** @kphp-json array_as_hashmap */
  public string $d = "d_value";
}

class C extends D {
  public string $a = "a_value";
}

class Demo {
  public C $obj;
}

function test_incorrect_tags_nested_objects() {
  JsonEncoder::encode(new Demo);
}

test_incorrect_tags();
test_incorrect_tags_nested_objects();
