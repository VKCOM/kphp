@kphp_should_warn
/Dangerous undefined behaviour \+, \[var = \$x\]/
<?php

try {
  $x = 10;
  throw new Exception();
} catch (Exception $e) {
  $y = $x++ + $x++;
  var_dump($y);
}
