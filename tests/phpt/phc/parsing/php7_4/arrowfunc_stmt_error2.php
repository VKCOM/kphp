@kphp_should_fail
/Bad expression in arrow function body/
<?php

$fn = fn() => {
  var_dump(1);
  var_dump(2);
};
var_dump($fn());
