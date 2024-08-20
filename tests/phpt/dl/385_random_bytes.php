@ok php8 k2_skip
<?php

function test_random_bytes() {
  for ($i = 1; $i < 50; $i += 1) {
    var_dump(strlen(random_bytes($i)));
  }
}

test_random_bytes();
