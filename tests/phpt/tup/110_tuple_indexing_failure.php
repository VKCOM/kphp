@kphp_should_fail
/but string type is passed/
/can not get element/
<?php

function test_invalid_indexing() {
  $tuple = tuple(1, 2, 3);
  echo $tuple["key"];
}

test_invalid_indexing();
