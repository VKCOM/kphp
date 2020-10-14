@kphp_should_fail
/TYPE INFERENCE ERROR/
<?php
require_once 'kphp_tester_include.php';

/**
 * @return shape(x:int, y?:int)
 */
function process() {
  if(1)
    return shape(['x' => 1, 'y' => 's']);
  return shape(['y' => 's']);
}

process();
