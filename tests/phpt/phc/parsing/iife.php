@ok
<?php

// Immediately invoked function expression.

function test() {
  $x = (function() { return 10; })();
  var_dump($x);

  $y = (function($x) { return $x + 1; })(534);
  var_dump($y);
}

test();
