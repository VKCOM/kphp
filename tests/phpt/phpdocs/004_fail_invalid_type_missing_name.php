@kphp_should_fail
/TYPE INFERENCE ERROR/
<?php

function demo() {
  /** @var string */
  $a = 4;
  var_dump($a);
}

demo();
