@kphp_should_fail php8
/return string from foo/
/but it's declared as @return never/
<?php

function foo(): never {
  return "hello"; // not permitted in a never function
}

foo();
