@kphp_should_fail
/TYPE INFERENCE ERROR/
<?php

$ints = [1];
$ints[] = 2;

function demo() {
  global $ints;
  /** @var int[] $ints */
  $ints[] = 'a';
}

demo();
