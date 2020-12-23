@kphp_should_fail
/assign array< string > to \$a/
<?php

function demo() {
  /** @var int $a */
  $a = 4;
  var_dump($a);
  $a = ['a'];
  var_dump($a);
}

demo();
