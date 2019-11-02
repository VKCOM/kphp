@ok
<?php
require_once 'Classes/autoload.php';

use Classes\A;

/**
 * @param A $a
 */
function p($a) {
  echo !$a ? "null" : $a->a, "\n";
  echo is_object($a) ? "is_object\n" : "";
}

$a = (new A)->setA(19);
echo !$a ? "null" : $a->a, "\n";
p($a);
$a = false;
echo !$a ? "null" : $a->a, "\n";
$a = null;
echo !$a ? "null" : $a->a, "\n";
p($a);
p(false);
p(null);

$aArr = array(new A, false, new A, null);
foreach ($aArr as $a2)
  p($a2);
