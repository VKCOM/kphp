@kphp_should_fail php8
<?php

function foo(
  $arg,
  $arg2,, // Multiple trailing commas are not allowed
) {
    echo $arg . $arg2 . "\n";
}