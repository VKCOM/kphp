@ok
<?php

function test_modify() {
  $datetime = new DateTime("2009-01-31 14:28:41");
  $new_datetime = $datetime->modify("+1 day");
  echo "After modification: " . $new_datetime->format("D, d M Y") . "\n";
  echo "After modification: " . $datetime->format("D, d M Y") . "\n";
}

function test_modify_immutable() {
  $datetime = new DateTimeImmutable("2009-01-31 14:28:41");
  $new_datetime = $datetime->modify("+1 day");
  echo "After modification: " . $new_datetime->format("D, d M Y") . "\n";
  echo "After modification: " . $datetime->format("D, d M Y") . "\n";
}

function test_modify_through_polymorphic_call(DateTimeInterface $datetime) {
  $new_datetime = $datetime->modify("+1 day");
  echo "After modification 1: " . $new_datetime->format("D, d M Y") . "\n";
  echo "After modification 1: " . $datetime->format("D, d M Y") . "\n";

  $new_datetime = $datetime->modify("+1 week 2 days 4 hours 2 seconds");
  echo "After modification 2: " . $new_datetime->format("D, d M Y H:i:s") . "\n";
  echo "After modification 2: " . $datetime->format("D, d M Y H:i:s") . "\n";

  $new_datetime = $datetime->modify("next Thursday");
  echo "After modification 3: " . $new_datetime->format("D, d M Y") . "\n";
  echo "After modification 3: " . $datetime->format("D, d M Y") . "\n";

  $new_datetime = $datetime->modify("last Sunday");
  echo "After modification 4: " . $new_datetime->format("D, d M Y") . "\n";
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
test_modify_immutable();
test_modify_through_polymorphic_call(new DateTime("2009-01-31 14:28:41"));
test_modify_through_polymorphic_call(new DateTimeImmutable("2009-01-31 14:28:41"));
test_modify_shift_date();
