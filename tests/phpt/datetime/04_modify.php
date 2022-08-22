@ok
<?php

function test_modify() {
  $datetime = new DateTime("2009-01-31 14:28:41");

  $datetime->modify("+1 day");
  echo "After modification 1: " . $datetime->format("D, d M Y") . "\n";

  $datetime->modify("+1 week 2 days 4 hours 2 seconds");
  echo "After modification 2: " . $datetime->format("D, d M Y H:i:s") . "\n";

  $datetime->modify("next Thursday");
  echo "After modification 3: " . $datetime->format("D, d M Y") . "\n";

  $datetime->modify("last Sunday");
  echo "After modification 4: " . $datetime->format("D, d M Y") . "\n";
}

function test_modify_shift_date() {
  $date = new DateTime('2000-12-31');

  // give 2001-01-31
  $date->modify('+1 month');
  echo $date->format('Y-m-d') . "\n";

  // give 2001-03-03
  $date->modify('+1 month');
  echo $date->format('Y-m-d') . "\n";
}

test_modify();
test_modify_shift_date();
