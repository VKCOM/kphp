@ok
<?php

/** @param string $x */
function ensure_string($x) {}

function test() {
  ensure_string(microtime());
  ensure_string(microtime(false));

  $t = microtime(false);
  ensure_string($t);
}

test();
