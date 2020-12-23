@kphp_should_fail
/Different @var for \$a exist/
<?php

function demo() {
  if(1) {
    /** @var int|false */
    $a = 4;
    if(1) $a = false;
    var_dump($a);
  } else {
    /** @var string */
    $a = 'a';
    var_dump($a);
  }
  echo $a;
}

demo();
