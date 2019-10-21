@kphp_should_fail
/TYPE INFERENCE ERROR/
<?php
function test(int ...$args) {
  var_dump($args);
}

test(1,2,3,4,5,6,7,"8");

