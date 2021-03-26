@ok
<?php

function arg(string $tag, int $value): int {
  echo "$tag:$value\n";
  return $value;
}

function test_set_value() {
  $array = [];
  $array[arg('key', 1)] = arg('val', 2);
  var_dump($array);
}

test_set_value();
