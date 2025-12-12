@ok
<?php

function test_date_interval_format(string $duration) {
  $interval = new DateInterval($duration);
  echo $interval->format('%y-%m-%d %h:%i:%s'), "\n";
  echo $interval->format(''), "\n";
  echo $interval->format('%'), "\n";
}

test_date_interval_format('P2Y4DT6H8M');
test_date_interval_format('P32D');
