@kphp_should_fail
/Expected '\)', found ','/
<?php

function test_many_extra_comma() {
  $tuple = tuple(1,,5);
  echo $tuple[0];
}

test_many_extra_comma();
