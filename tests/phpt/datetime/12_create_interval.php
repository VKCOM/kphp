@ok k2_skip
<?php

function test_constructor_date_interval() {
  $durations = ['P1D', 'P2W', 'P3M', 'P4Y', 'P1Y1D', 'P1DT12H', 'PT3600S', 'P0001-12-04T10:05:55'];

  foreach ($durations as $duration) {
    $i = new DateInterval($duration);
    echo $i->format('%y-%m-%d %h:%i:%s'), "\n";
  }
}

function test_constructor_date_interval_range() {
  $durations = ['2003-02-15T00:00:00Z/P2M', '2007-03-01T13:00:00Z/2008-05-11T15:30:00Z',
    '2007-03-01T13:00:00Z/P1Y2M10DT2H30M', 'P1Y2M10DT2H30M/2008-05-11T15:30:00Z', 'R5/2008-03-02T13:00:00Z/P1Y2M10DT2H30M'];

  foreach ($durations as $duration) {
    $i = new DateInterval($duration);
    echo $i->format('%y-%m-%d %h:%i:%s'), "\n";
  }
}

function test_constructor_date_interval_invalid_duration() {
  $intervals = ['', 'wat', '7', '7D', 'P4', 'P1L', 'P0001-12-0410:05:55', 'P0001-12-04T10:05'];

  foreach ($intervals as $interval) {
    try {
      $i = new DateInterval($interval);
      var_dump($i->format('%y-%m-%d'));
    } catch (Exception $e) {
     var_dump($e->getMessage());
     var_dump(DateTime::getLastErrors());
    }
  }
}

test_constructor_date_interval();
test_constructor_date_interval_range();
test_constructor_date_interval_invalid_duration();
