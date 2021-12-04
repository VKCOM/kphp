@ok php8
<?php

// See https://wiki.php.net/rfc/trailing_comma_in_parameter_list

function foo(
  $arg,
  $arg2, // Ok
) {
    echo $arg . $arg2 . "\n";
}

