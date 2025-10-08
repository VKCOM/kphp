@ok
<?php

function test_inf() {
  $values = [exp(800), pow(0, -2), INF, -INF, INF + INF, INF * INF, INF - INF, INF / INF];

  foreach ($values as $value) {
    echo "is_nan      ($value) = ", is_nan($value), "\n";
    echo "is_infinite ($value) = ", is_infinite($value), "\n";
    echo "is_finite   ($value) = ", is_finite($value), "\n";
  }
}

test_inf();
