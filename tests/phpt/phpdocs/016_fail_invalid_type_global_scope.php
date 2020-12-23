@kphp_should_fail
/insert string into \$ints\[\]/
<?php

/** @var int[] $ints */
$ints = [1];
$ints[] = 2;

function demo() {
  global $ints;
  $ints[] = 'a';
}

demo();
