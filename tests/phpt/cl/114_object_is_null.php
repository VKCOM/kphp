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
$a = null;
echo !$a ? "null" : $a->a, "\n";
p($a);
p(null);

$aArr = array(new A, null, new A);
foreach ($aArr as $a2)
  p($a2);
