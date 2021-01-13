@kphp_should_fail
/Function returns void and false simultaneously/
<?php


function f() {
  if (1) return false;
}

f();
