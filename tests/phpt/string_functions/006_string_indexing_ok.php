@ok
<?php
function optional_int(): ?int {
  return 1;
}

function test_valid_indexing() {
  $string = "Hello";

  echo $string[true];
  echo $string[false];
  echo $string[1];

  echo $string[optional_int()];
}

test_valid_indexing();
