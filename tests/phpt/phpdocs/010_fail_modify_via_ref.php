@kphp_should_fail
/TYPE INFERENCE ERROR/
<?php

function modify(&$a) {
  $a = 'str';
}

function demo() {
  /** @var int $a */
  echo "test modify type via ref\n";
  $a = 0;
  modify($a);
}

demo();
