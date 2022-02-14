@ok php8
<?php

function get_range($i) {
  return match (true) {
    $i >= 50 => '50+',
    $i >= 40 => '40-50',
    $i >= 30 => '30-40',
    $i >= 20 => '20-30',
    $i >= 10 => '10-20',
    default => '0-10',
  };
}

echo get_range(22) . "\n";
echo get_range(0) . "\n";
echo get_range(59) . "\n";
echo get_range(13) . "\n";
echo get_range(39) . "\n";
echo get_range(40) . "\n";
