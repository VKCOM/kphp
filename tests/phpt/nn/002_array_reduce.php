@ok
<?php
$t = array(1, 2, 3, 4, 5);

function multiply($carry, $item) {
  if ($carry === NULL) {
    return $item;
  }
  $carry = $carry * $item;
  return $carry;
}

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
var_dump(array_reduce($t, "multiply"));
var_dump(array_reduce(array(), "multiply", 5));
var_dump(array_reduce($z, "addall"));
var_dump(array_reduce(array(1, 2, 3), function($carry, $item) {
  if ($carry === NULL) return $item;
  return $carry + $item;
}), 7);
