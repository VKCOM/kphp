@kphp_should_fail
/assign string to \$a/
<?php

function demo() {
  /** @var int $a */
  if(1) {
    $a = 4;
    var_dump($a);
  } else {
    $a = 'a';
    var_dump($a);
  }
}

demo();
