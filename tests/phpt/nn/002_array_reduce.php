@ok
<?php
$t = array(1, 2, 3, 4, 5);

/**
 * @kphp-required
 */
function multiply($carry, $item) {
  if ($carry === NULL) {
    return $item;
  }
  $carry = $carry * $item;
  return $carry;
}

/**
 * @kphp-required
 */
function addall($carry, $item) {
  if ($carry === NULL) {
    $carry = array();
  }
  foreach ($item as $i) {
    $carry[] = $i;
  }
  return $carry;
}

$z = array(array(1), array("a"), array(3));
var_dump(array_reduce($t, "multiply", NULL));
$a = array(1);
unset($a[0]);
var_dump(array_reduce($a, "multiply", 5));
var_dump(array_reduce($z, "addall", NULL));
var_dump(array_reduce(array(1, 2, 3), function($carry, $item) {
  if ($carry === NULL) return $item;
  return $carry + $item;
}, 1 ? 7 : null));
