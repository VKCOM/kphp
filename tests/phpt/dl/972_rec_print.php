@ok no_php
<?php
  function f($n) {
    if ($n <= 0) {
      return 0;
    }
    echo "[$n] = {$t = f($n - 1)}\n";
    return $n * $n;
  }
  f(10);
?>