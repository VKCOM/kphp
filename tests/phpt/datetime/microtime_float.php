@ok
<?php

/** @param float $x */
function ensure_float($x) {}

function test() {
  ensure_float(microtime(true));

  $t = microtime(true);
  ensure_float($t);
}

test();
