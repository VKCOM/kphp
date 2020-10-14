@kphp_should_fail
/@param for unexisting argument \$x/
<?php

/**
 * @param string $x
 */
function printName($name) {
  echo $name, "\n";
}

printName('d');
