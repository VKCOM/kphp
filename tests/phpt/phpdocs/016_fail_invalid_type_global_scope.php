@kphp_should_fail
/TYPE INFERENCE ERROR/
<?php

/** @var int[] $ints */
$ints = [1];
$ints[] = 2;

function demo() {
  global $ints;
  $ints[] = 'a';
}

demo();
