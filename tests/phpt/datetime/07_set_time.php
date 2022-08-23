@ok
<?php

function test_set_time() {
  $date = new DateTime('2001-01-01');

  $new_date = $date->setTime(14, 55);
  echo $date->format('Y-m-d H:i:s') . "\n";
  echo $new_date->format('Y-m-d H:i:s') . "\n";

  $date->setTime(14, 55, 24);
  echo $date->format('Y-m-d H:i:s') . "\n";
}

function test_set_time_exceed_range() {
  $date = new DateTime('2001-01-01');

  $date->setTime(14, 55, 24);
  echo $date->format('Y-m-d H:i:s') . "\n";

  $date->setTime(14, 55, 65);
  echo $date->format('Y-m-d H:i:s') . "\n";

  $date->setTime(14, 65, 24);
  echo $date->format('Y-m-d H:i:s') . "\n";

  $date->setTime(25, 55, 24);
  echo $date->format('Y-m-d H:i:s') . "\n";
}

test_set_time();
test_set_time_exceed_range();
