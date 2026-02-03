@kphp_runtime_should_warn
/DateInterval::createFromDateString\(\): Unknown or bad format \(\) at position 0 \( \): Empty string/
/DateInterval::createFromDateString\(\): Unknown or bad format \(last what\) at position 0 \(l\): The timezone could not be found in the database/
<?php

function test_create_interval_wrong_date(string $datetime) {
  $interval = DateInterval::createFromDateString($datetime);
  var_dump((bool)$interval);
  var_dump(DateTime::getLastErrors());
}

test_create_interval_wrong_date('');
test_create_interval_wrong_date('last what');
