@kphp_should_fail
/insert string into \$ints\[\]/
<?php

$ints = [1];
$ints[] = 2;

function demo() {
  global $ints;
  /** @var int[] $ints */
  $ints[] = 'a';
}

demo();
