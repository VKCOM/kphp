@kphp_should_fail
<?php
function foo(): int {
  return "something";
}

$a = foo();

