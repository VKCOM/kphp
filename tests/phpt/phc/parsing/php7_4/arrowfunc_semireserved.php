@ok php7_4
<?php

class ExampleField {
  public $fn = "fn field";
}

$obj2 = new ExampleField();
var_dump($obj2->fn);

class ExampleConst {
  const fn = "fn const";
}

var_dump(ExampleConst::fn);

// TODO: enable these tests
/*
class ExampleMethod {
  public function fn() { echo __METHOD__; }
}

$obj1 = new ExampleMethod();
var_dump($obj1->fn());

class ExampleStatic {
  public static function fn() { echo __METHOD__; }

  public static $fn = "fn static field";
}

var_dump(ExampleStatic::fn());
var_dump(ExampleStatic::$fn);
*/
