@kphp_should_fail
/assign mixed to \$a/
<?php

/**@param mixed $p*/
function modify(&$p) {
  $p = 'str';
}

function demo() {
  /** @var int $a */
  echo "test modify type via ref\n";
  $a = 0;
  modify($a);
}

demo();
