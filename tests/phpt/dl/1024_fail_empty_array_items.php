@kphp_should_fail
/array with empty elements/
<?php

function demo() {
  list($a, , $c) = [1, , 3];
}

demo();

