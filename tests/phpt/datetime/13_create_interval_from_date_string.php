@ok
<?php

function test_create_interval_from_date_string() {
  $datetimes = ['1 day', '2 weeks', '3 months', '4 years', '1 year + 1 day', '1 day + 12 hours', '3600 seconds'];

  foreach ($datetimes as $datetime) {
    $i = DateInterval::createFromDateString($datetime);
    echo $i->format('%y-%m-%d %h:%i:%s'), "\n";
  }
}

function test_parse_combinations() {
  $i = DateInterval::createFromDateString('62 weeks + 1 day + 2 weeks + 2 hours + 70 minutes');
  echo $i->format('%d %h %i'), "\n";

  $i = DateInterval::createFromDateString('1 year - 10 days');
  echo $i->format('%y %d'), "\n";
}

function test_special_relative_intervals() {
  $i = DateInterval::createFromDateString('last day of next month');
  echo $i->format('%d %h %i'), "\n";

  $i = DateInterval::createFromDateString('last weekday');
  echo $i->format('%d %h %i'), "\n";
}

test_create_interval_from_date_string();
test_parse_combinations();
test_special_relative_intervals();
