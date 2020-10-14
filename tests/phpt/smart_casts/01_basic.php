@ok
<?php

function f_int(int $x) {
  echo "Hi, i'm int!";
}

function f_string(string $x) {
  echo "Hi, i'm int!";
}

function f_array(array $x) {
  echo "Hi, i'm array!";
}

/**
 * @param mixed $x
 */
function f($x) {
  if (is_int($x)) {
    f_int($x);
  } else if (is_string($x)) {
    f_string($x);
  } else if (is_array($x)) {
    f_array($x);
  }
  f_int(is_int($x) ? $x : 0);
  var_dump($x);
}

f(1);
f("1");
f([1]);
