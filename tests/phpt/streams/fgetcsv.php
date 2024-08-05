@ok k2_skip
<?php

function test_empty_escape() {
  $stream = fopen(__DIR__ . '/csv_input.txt', 'r');
  while (($data = fgetcsv($stream, 0, ',', '"', '')) !== false) {
      var_dump($data);
  }
  fclose($stream);
}

test_empty_escape();
