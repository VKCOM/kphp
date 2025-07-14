@ok benchmark
<?php
#ifndef KPHP
function create_vector ($n, $x) {
  $res = array();
  for ($i = 0; $i < $n; $i++) {
    $res[] = $x;
  }
  return $res;
}
#endif

$maxp = 3000000;
$f = create_vector ($maxp, false);
for ($i = 2; $i < $maxp; $i++) {
  if (!$f[$i]) {
    $primes[] = $i;
    if ($i <= $maxp / $i)
    for ($j = $i * $i; $j < $maxp; $j += $i) {
      $f[$j] = true;
    }
  }
}
$n = sizeof ($primes);
echo ("sizeof (primes) = $n\n");
?>
