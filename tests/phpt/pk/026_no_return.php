@ok
<?php

/**
 * @kphp-no-return
 */ 
function f() {
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
