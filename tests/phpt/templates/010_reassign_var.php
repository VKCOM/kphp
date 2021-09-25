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


class AFiel {
    public $fiel = 0;
    function fa() { echo "AFiel fa\n"; }
}
class BFiel {
    public $fiel = 1;
}
/**
 * @kphp-template T $obj
 * @kphp-return T
 */
function same($obj) {
    return $obj;
}
function demoNestedTplInfer() {
    $a = new AFiel;
    same($a)->fa();
    same(same($a))->fa();
    $a1 = same($a);
    $a2 = same($a1);
    $a3 = same($a2);
    $a1->fiel = 9;
    echo $a->fiel;
    echo $a1->fiel;
    echo $a2->fiel;
    echo $a3->fiel;
    $b = new BFiel;
    same(same(same($b)))->fiel = 10;
    echo same(same($b))->fiel;
}
demoNestedTplInfer();
