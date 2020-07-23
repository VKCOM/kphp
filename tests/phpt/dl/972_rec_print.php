@ok
<?php
#ifndef KittenPHP
function f($n) {
  if ($n <= 0) {
    return 0;
  }

  $t = f($n - 1);
  echo "[$n] = $t\n";
  return $n * $n;
}

f(10);

exit;
?>
#endif

/**
 * @kphp-infer
 * @param int $n
 * @return int
 */
function f($n) {
  if ($n <= 0) {
    return 0;
  }

  echo "[$n] = {$t = f($n - 1)}\n";
  return $n * $n;
}

f(10);
