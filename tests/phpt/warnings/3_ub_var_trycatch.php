@kphp_should_warn k2_skip
/Dangerous undefined behaviour \+, \[var = \$x\]/
<?php

try {
  $x = 10;
  throw new Exception();
} catch (Exception $e) {
  $y = $x++ + $x++;
  var_dump($y);
}
