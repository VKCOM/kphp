@ok k2_skip
<?php

function test_empty_escape() {
  $stream = fopen('php://stdout', 'w');
  $rows = [
    ['\\'],
    ['\\"']
  ];
  foreach ($rows as $row) {
    fputcsv($stream, $row, ',', '"', '');
  }
}

test_empty_escape();
