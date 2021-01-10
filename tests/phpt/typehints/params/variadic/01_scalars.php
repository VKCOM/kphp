@kphp_should_fail
/pass mixed\[\] to argument \$args of test/
/but it's declared as @param int\[\]/
<?php
function test(int ...$args) {
  var_dump($args);
}

test(1,2,3,4,5,6,7,"8");

