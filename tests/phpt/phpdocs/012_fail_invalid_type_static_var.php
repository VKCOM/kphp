@kphp_should_fail
/insert string into \$ints\[\]/
<?php

function demo() {
  /** @var int[] $ints */
  static $ints = [];
  $ints[] = 'a';
  var_dump($ints);
}

demo();
