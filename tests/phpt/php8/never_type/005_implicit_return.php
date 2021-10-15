@kphp_should_fail php8
/return void from foo/
/but it's declared as @return never/
<?php

function foo(): never {
  if (false) {
    throw new Exception('bad');
  }
}

foo();
