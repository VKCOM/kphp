@kphp_should_fail k2_skip
/really can throw an exception/
<?php

function cc() {
  fetch_int();      // see functions.txt, it can throw
}

/** @kphp-should-not-throw */
function demo() {
  echo 1;
  cc();
}

demo();

