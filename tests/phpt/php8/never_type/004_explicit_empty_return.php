@kphp_should_fail php8
/return void from foo/
/but it's declared as @return never/
<?php

function foo(): never {
  return; // not permitted in a never function
}

foo();
