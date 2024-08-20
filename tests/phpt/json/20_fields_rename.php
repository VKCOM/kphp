@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class MyJsonEncoder extends JsonEncoder {
  const rename_policy = "camelCase";
}

class C {
  public string $c_foo;

  function init() {
    $this->c_foo = "c_foo";
  }
}

class B extends C {
  public string $b_foo;

  function init() {
    $this->b_foo = "b_foo";
    $this->c_foo = "c_foo";
  }
}

class A extends B {
  public string $a_foo;

  function init() {
    $this->a_foo = "a_foo";
    $this->b_foo = "b_foo";
    $this->c_foo = "c_foo";
  }
}

class Foo {
  public string $foo_bar;
  public string $bar_baz;

  function init() {
    $this->foo_bar = "a";
    $this->bar_baz = "b";
  }
}

class C1 {
  public string $c_foo;
}

/** @kphp-json rename_policy=snake_case */
class B1 extends C1 {
  public string $b_foo;
}

class A1 extends B1 {
  public string $a_foo;
}

/** @kphp-json rename_policy=snake_case */
class Foo1 {
  public string $foo_bar;
  public string $bar_baz;
}


function test_rename_policy_decode() {
  $json = '{"foo_bar":"a","bar_baz":"b"}';
  $my_json = '{"fooBar":"a","barBaz":"b"}';
  $dump_foo = to_array_debug(JsonEncoder::decode($json, Foo::class));
  $dump_my_foo = to_array_debug(MyJsonEncoder::decode($my_json, Foo::class));
  var_dump($dump_foo);
  var_dump($dump_my_foo);
}

function test_rename_policy_inheritance_decode() {
  $json_a = '{"a_foo":"a_foo","b_foo":"b_foo","c_foo":"c_foo"}';
  $json_b = '{"b_foo":"b_foo","c_foo":"c_foo"}';
  $json_c = '{"c_foo":"c_foo"}';
  $my_json_a = '{"aFoo":"a_foo","bFoo":"b_foo","cFoo":"c_foo"}';
  $my_json_b = '{"bFoo":"b_foo","cFoo":"c_foo"}';
  $my_json_c = '{"cFoo":"c_foo"}';

  $dump_json_a = to_array_debug(JsonEncoder::decode($json_a, A::class));
  $dump_json_b = to_array_debug(JsonEncoder::decode($json_b, B::class));
  $dump_json_c = to_array_debug(JsonEncoder::decode($json_c, C::class));
  $dump_my_json_a = to_array_debug(MyJsonEncoder::decode($my_json_a, A::class));
  $dump_my_json_b = to_array_debug(MyJsonEncoder::decode($my_json_b, B::class));
  $dump_my_json_c = to_array_debug(MyJsonEncoder::decode($my_json_c, C::class));

  var_dump($dump_json_a);
  var_dump($dump_json_b);
  var_dump($dump_json_c);
  var_dump($dump_my_json_a);
  var_dump($dump_my_json_b);
  var_dump($dump_my_json_c);
}

function test_rename_policy() {
  $foo = new Foo;
  $foo->init();
  $json = JsonEncoder::encode($foo);
  $my_json = MyJsonEncoder::encode($foo);
  $dump_foo = to_array_debug(JsonEncoder::decode($json, Foo::class));
  $dump_my_foo = to_array_debug(MyJsonEncoder::decode($my_json, Foo::class));
  var_dump($dump_foo);
  var_dump($dump_my_foo);
}

function test_rename_policy_inheritance() {
  $a = new A;
  $b = new B;
  $c = new C;
  $a->init();
  $b->init();
  $c->init();
  $json_a = JsonEncoder::encode($a);
  $json_b = JsonEncoder::encode($b);
  $json_c = JsonEncoder::encode($c);
  $my_json_a = MyJsonEncoder::encode($a);
  $my_json_b = MyJsonEncoder::encode($b);
  $my_json_c = MyJsonEncoder::encode($c);

  $dump_json_a = to_array_debug(JsonEncoder::decode($json_a, A::class));
  $dump_json_b = to_array_debug(JsonEncoder::decode($json_b, B::class));
  $dump_json_c = to_array_debug(JsonEncoder::decode($json_c, C::class));
  $dump_my_json_a = to_array_debug(MyJsonEncoder::decode($my_json_a, A::class));
  $dump_my_json_b = to_array_debug(MyJsonEncoder::decode($my_json_b, B::class));
  $dump_my_json_c = to_array_debug(MyJsonEncoder::decode($my_json_c, C::class));

  var_dump($dump_json_a);
  var_dump($dump_json_b);
  var_dump($dump_json_c);
  var_dump($dump_my_json_a);
  var_dump($dump_my_json_b);
  var_dump($dump_my_json_c);
}

function test_rename_policy_with_tags_decode() {
  $json = '{"foo_bar":"a","bar_baz":"b"}';
  $my_json = '{"foo_bar":"a","bar_baz":"b"}';
  $dump_foo = to_array_debug(JsonEncoder::decode($json, Foo1::class));
  $dump_my_foo = to_array_debug(MyJsonEncoder::decode($my_json, Foo1::class));
  var_dump($dump_foo);
  var_dump($dump_my_foo);
}

function test_rename_policy_inheritance_with_tags_decode() {
  $json_a = '{"a_foo":"a_foo","b_foo":"b_foo","c_foo":"c_foo"}';
  $json_b = '{"b_foo":"b_foo","c_foo":"c_foo"}';
  $json_c = '{"c_foo":"c_foo"}';
  $my_json_a = '{"aFoo":"a_foo","b_foo":"b_foo","cFoo":"c_foo"}';
  $my_json_b = '{"b_foo":"b_foo","cFoo":"c_foo"}';
  $my_json_c = '{"cFoo":"c_foo"}';

  $dump_json_a = to_array_debug(JsonEncoder::decode($json_a, A1::class));
  $dump_json_b = to_array_debug(JsonEncoder::decode($json_b, B1::class));
  $dump_json_c = to_array_debug(JsonEncoder::decode($json_c, C1::class));
  $dump_my_json_a = to_array_debug(MyJsonEncoder::decode($my_json_a, A1::class));
  $dump_my_json_b = to_array_debug(MyJsonEncoder::decode($my_json_b, B1::class));
  $dump_my_json_c = to_array_debug(MyJsonEncoder::decode($my_json_c, C1::class));

  var_dump($dump_json_a);
  var_dump($dump_json_b);
  var_dump($dump_json_c);
  var_dump($dump_my_json_a);
  var_dump($dump_my_json_b);
  var_dump($dump_my_json_c);
}

test_rename_policy_decode();
test_rename_policy_inheritance_decode();
test_rename_policy();
test_rename_policy_inheritance();
test_rename_policy_with_tags_decode();
test_rename_policy_inheritance_with_tags_decode();
