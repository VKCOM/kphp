@kphp_should_fail
/TYPE INFERENCE ERROR/
<?php

function demo() {
  /** @var int $a */
  $a = 4;
  var_dump($a);
  $a = 'a';
  var_dump($a);
}

demo();
