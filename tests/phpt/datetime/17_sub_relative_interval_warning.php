@kphp_runtime_should_warn
/DateTime::sub\(\): Only non-special relative time specifications are supported for subtraction/
/DateTimeImmutable::sub\(\): Only non-special relative time specifications are supported for subtraction/
<?php

function test_sub_relative_interval_warning(DateTimeInterface $date) {
  $relative_interval = DateInterval::createFromDateString('third Tuesday of next month');
  $new_date = $date->sub($relative_interval);

  echo $date->format(DateTimeInterface::ISO8601), "\n";
  echo $new_date->format(DateTimeInterface::ISO8601), "\n";
}

test_sub_relative_interval_warning(new DateTime('2010-03-07 13:21:38 UTC'));
test_sub_relative_interval_warning(new DateTimeImmutable('2010-03-07 13:21:38 UTC'));
