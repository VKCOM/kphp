@ok
<?php
function test(int $value = null) {
  var_dump($value);
}

test(42);
test(null);

