@kphp_should_fail
KPHP_SHOW_ALL_TYPE_ERRORS=1
/return shape<y:string> from process/
/return shape<x:int, y:string> from process/
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
