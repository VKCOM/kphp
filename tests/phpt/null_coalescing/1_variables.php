@ok k2_skip
<?php

/**
 * @return string[]
 */
function foobar() {
  echo "foobar called\n";
  return ['a'];
}

function test_variable() {
  /** @var int */
  $v0 = 7;
  /** @var null|int */
  $v1 = 1 ? null : $v0;
  /** @var mixed */
  $v3 = 0 ? $v1 : "xxxx";
  /** @var string */
  $v4 = "";

  var_dump($v0 ?? 2);
  var_dump($v1 ?? 3);
  var_dump($v1 ?? $v0);
  var_dump($v3 ?? $v1);
  var_dump($v4 ?? $v1);
  var_dump($v4 ?? "fooo");
  var_dump($v1 ?? $v4);
  var_dump($v0 ?? foobar());
  var_dump($v3 ?? foobar());
  var_dump($v3 ?? $v1 ?? $v0 ?? foobar());
}

test_variable();
