@ok
<?php

/** @var int[] $ints */
$ints = [1];
$ints[] = 2;

function demo() {
  global $ints;
  $ints[] = 3;
}

demo();
