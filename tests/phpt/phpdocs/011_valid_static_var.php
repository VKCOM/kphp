@ok
<?php

function demo() {
  /** @var int[] $ints */
  static $ints = [];
  $ints[] = 1;
  var_dump($ints);
}

demo();
