@ok benchmark k2_skip
<?php
  function hash2($n) {
    for ($i = 0; $i < $n; $i++) {
      $hash1["foo_$i"] = 1;
    }
    $res = 0;
    for ($i = $n; $i > 0; $i--) {
      foreach($hash1 as $value) {
        $res += $value;
      }
    }
    print "{$res}\n";
  }

  hash2(10000);
