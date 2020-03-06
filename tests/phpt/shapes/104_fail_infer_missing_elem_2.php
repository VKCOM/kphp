@kphp_should_fail
/TYPE INFERENCE ERROR/
<?php
require_once 'polyfills.php';

/**
 * @kphp-infer
 * @return shape(x:int, y?:int)
 */
function process() {
  if(1)
    return shape(['x' => 1, 'y' => 's']);
  return shape(['y' => 's']);
}

process();
