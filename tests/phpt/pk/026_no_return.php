@ok k2_skip
<?php

/**
 * @kphp-no-return
 */ 
function f() {
  exit(1);
}

/**
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
