@ok
<?php

function test1($x) {
  switch ($x) {
  case '1':
    return 1;
  default:
    return 0;
  }
}

function test2(string $s) {
  switch ($s) {
  case '5':
    return 1;
  default:
    return 0;
  }
}

function test3(string $s) {
  switch ($s) {
  case ' 5':
    return 1;
  default:
    return 0;
  }
}

$values = [
  1, 2, 5, 0, 10, 20,
  '1', '2', '5', '0', '10', '20',
];
$all_values = [];
foreach ($values as $x) {
  $all_values[] = $x;
  if (!is_string($x)) {
    continue;
  }
  $all_values[] = "\n" . $x;
  $all_values[] = $x . "\n";
  $all_values[] = ' ' . $x;
  $all_values[] = $x . ' ';
  $all_values[] = '+' . $x;
  $all_values[] = '-' . $x;
  $all_values[] = ' +' . $x;
  $all_values[] = ' -' . $x;
  $all_values[] = ' +' . $x . ' ';
  $all_values[] = ' -' . $x . ' ';
}
foreach ($all_values as $i => $x) {
  echo "[$i] test1(`$x`) => " . test1($x) . "\n";
  if (is_string($x)) {
    echo "[$i] test2(`$x`) => " . test2((string)$x) . "\n";
    echo "[$i] test3(`$x`) => " . test3((string)$x) . "\n";
  }
}
