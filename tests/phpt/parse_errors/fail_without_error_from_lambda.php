@kphp_should_fail
/Duplicate 'use' at the top of the file/
!/Failed to parse second argument in/
<?php

use A\Foo;
use A\Foo;

$fn = function($x) {
  echo $x;
};

$fn('hello');
