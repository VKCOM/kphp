@kphp_should_fail
/Unsetting/
<?php

$a = [1, 2, 3];
foreach ($a as &$x) {
  unset($x);
}
