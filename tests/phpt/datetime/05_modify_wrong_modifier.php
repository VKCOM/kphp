@kphp_runtime_should_warn k2_skip
/DateTime::modify\(\): Failed to parse time string \(\+1 day \+four month\) at position 7 \(\+\): Unexpected character/
/DateTimeImmutable::modify\(\): Failed to parse time string \(\+1 day \+four month\) at position 7 \(\+\): Unexpected character/
<?php

function test_modify_wrong_modifier(DateTimeInterface $date) {
  $new_date = $date->modify('+1 day +four month');
  var_dump((bool)$new_date);
  var_dump((bool)$date);
  echo $date->format("D, d M Y") . "\n";
  var_dump(DateTime::getLastErrors());
}

test_modify_wrong_modifier(new DateTime('2000-12-31'));
test_modify_wrong_modifier(new DateTimeImmutable('2000-12-31'));
