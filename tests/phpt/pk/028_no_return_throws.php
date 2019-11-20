@kphp_should_fail
/no_return
<?php

/**
 * @kphp-no-return
 */ 
function f() {
  throw new Exception();
  exit(1);
}

/**
 * @kphp-infer 
 * @return int
 */
function g() {
  if (1) {
    return 5;
  }
  f();
  return "5";
}

g();
