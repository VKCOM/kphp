@kphp_should_fail
/return float from f/
/declared as @return int\|string/
<?php

/** @return int|string */
function f($cond) {
  if ($cond === 1) {
    return 1; // OK
  }
  if ($cond === 2) {
    return 'str'; // OK
  }
  return 2.5; // error
}

f(1);
