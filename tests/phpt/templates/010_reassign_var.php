@ok
<?php

class A {
  public $data = "a";
}


class B {
  public $data = "b";
}

/**
 * @kphp-template T $obj
 */
function set_hello($obj) {
  $obj->data = "hello";
  return $obj;
}

/**
 * @kphp-template T $obj
 */
function add_world($obj) {
  $obj->data .= " world!";
  return $obj;
}

/**
 * @kphp-template T $obj
 */
function var_reassign_template($obj) {
  $obj->data = "";
  $obj = set_hello($obj);
  $obj = add_world($obj);
  var_dump($obj->data);
}


function test_var_reassign() {
  $a = new A;
  $a = set_hello($a);
  $a = add_world($a);
  var_dump($a->data);

  $b = new B;
  $b = set_hello($b);
  $b = add_world($b);
  var_dump($b->data);
}

function test_var_reassign_template() {
  var_reassign_template(new A);
  var_reassign_template(new B);
}

test_var_reassign();
test_var_reassign_template();
