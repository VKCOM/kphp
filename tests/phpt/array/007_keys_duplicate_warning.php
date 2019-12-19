@kphp_should_warn
/Got array duplicate keys \['5'\] in 'static A::\$x' init value/
/Got array duplicate keys \['BB'\] in 'A::\$y' init value/
/\$p = \["A" => "y", "A" => "x", A::E => "E", 5 => 5\]/
/Got array duplicate keys \['5', 'A'\]/
/\$y = \[A::F => "5 str", 5 => "5"\]/
/Got array duplicate keys \['5'\]/
/var_dump\(A::X\);/
/Got array duplicate keys \['BB'\]/
/Got array duplicate keys \['BB', 'XXX'\] in 'static \$x' init value/
<?php

class A {
  const B = "BB";
  const C = 5;
  const D = "BB";
  const E = 5;
  const F = "5";

  const X = [A::B => "AB", A::D => "AD"];

  public static $x = [5 => 1, A::C => 2];

  public $y = [A::D => "D", "BB" => "B"];
}

function test_duplicate_keys($p = ["A" => "y", "A" => "x", A::E => "E", 5 => 5]) {
  static $x = [A::D => "y", A::B => "x", "XXX" => "X1", "BB" => "B", "XXX" => "X2"];

  $y = [A::F => "5 str", 5 => "5"];

  var_dump($p);
  var_dump($x);
  var_dump($y);
  var_dump(A::X);
  var_dump(A::$x);
  var_dump((new A)->y);
}

test_duplicate_keys();
