@kphp_should_fail
/assign int to \$a/
<?php

function demo() {
  /** @var string */
  $a = 4;
  var_dump($a);
}

demo();
