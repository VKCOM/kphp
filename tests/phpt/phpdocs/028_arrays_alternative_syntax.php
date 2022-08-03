@ok
<?php

/** @param array<int> $a */
function f1($a) {
  var_dump($a);
}

/** @param array<int, int> $a */
function f2($a) {
  var_dump($a);
}

/** @param array<string, int> $a */
function f3($a) {
  var_dump($a);
}

f1([1, 2, 3]);
f2([1, 2, 3]);
f3([1, 2, 3]);
