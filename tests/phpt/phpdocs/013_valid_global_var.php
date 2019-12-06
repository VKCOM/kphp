@ok
<?php

$ints = [1];
$ints[] = 2;

function demo() {
  global $ints;
  /** @var int[] $ints */
  $ints[] = 1;
}

demo();
