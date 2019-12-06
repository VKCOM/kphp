@kphp_should_fail
/TYPE INFERENCE ERROR/
<?php

function demo() {
  /** @var int[] $ints */
  static $ints = [];
  $ints[] = 'a';
  var_dump($ints);
}

demo();
