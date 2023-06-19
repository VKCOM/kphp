@ok php8
<?php

function wordify($x) {
  return match ($x) {
    0 => 'Zero',
    1 => 'One',
    2 => 'Two',
    3 => 'Three',
    4 => 'Four',
    5 => 'Five',
    6 => 'Six',
    7 => 'Seven',
    8 => 'Eight',
    9 => 'Nine',
    default => 'default',
  };
}

for ($i = 0; $i <= 9; $i++) {
  echo wordify($i) . "\n";
}
