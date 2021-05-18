<?php

namespace Foo\Bar;

class Baz {
  public function f() {
    return strlen('fdfd');
  }

  public static function g() {
    return strtotime('1032');
  }

  public static function use_array_merge() {
    return array_merge([1], [2]);
  }
}

var_dump(strlen('123'));
$baz = new Baz();
var_dump($baz->f());
var_dump(Baz::g());
var_dump(Baz::use_array_merge());

function test_array_merge() {
  $xs = [1];
  var_dump(array_merge([1], $xs));
}

function test_array_filter() {
  $a = [1, 2, 3, 4];
  $a = array_filter($a, function($x) { return $x > 4; });
  var_dump($a);
}

function test_array_map() {
  $a = [1, 2, 3, 4];
  $a = array_map(function($x) { return $x + 1; }, $a);
  var_dump($a);
}

test_array_merge();
test_array_filter();
test_array_map();
