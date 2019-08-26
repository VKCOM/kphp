@kphp_should_fail
/mixing void/
<?php


function f() {
  if (1) return false;
}

f();
