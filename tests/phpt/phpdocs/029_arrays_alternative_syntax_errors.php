@kphp_should_fail
/array<\.\.\.> has to be parameterized with one type or two types at most/
/array<\.\.\.> has to be parameterized with one type or two types at most/
<?php

/** @param array<bool, string, int> $a */
function f1($a) {
  var_dump($a);
}

/** @param array<> $a */
function f2($a) {
  var_dump($a);
}

f1([1, 2, 3]);
f2([1, 2, 3]);
