@kphp_should_fail
/function use list: expected varname, found 15/
<?php

function f() {
  return function($x) use(15) {};
}

f();
