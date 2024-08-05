@kphp_should_fail k2_skip
/tuple\(int, int\) is not comparable and cannot be sorted/
/shape\(x:int\) is not comparable and cannot be sorted/
/C is not comparable and cannot be sorted/
<?php
require_once 'kphp_tester_include.php';

function test_tuple_elems() {
  $a = [tuple(1, 2), tuple(2, 3)];
  sort($a);
}

function test_shape_elems() {
  $a = [shape(['x' => 1]), shape(['x' => 2])];
  rsort($a);
}

class C {}

function test_instance_elems() {
  $a = [new C(), new C()];
  asort($a);
}

test_tuple_elems();
test_shape_elems();
test_instance_elems();
