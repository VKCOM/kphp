@ok
<?php

function test_constructor_valid_date() {
  var_dump((new DateTime())->format('Y-m-d'));
  var_dump((new DateTime(''))->format('Y-m-d'));
  var_dump((new DateTime('now'))->format('Y-m-d'));

  $dates = ['2000-01-01', '20000101 23:32:12', '2000-01-01 23:32:12',
    '2000-01-01 233212', '2000-01-01 23:32', '2000-01-01 2332', '23:32:12', '23:32', '233212', '2332'];

  foreach ($dates as $date) {
    var_dump((new DateTime($date))->format("Y-m-d H:i:s"));
    var_dump(DateTime::getLastErrors());
  }
}

function test_constructor_invalid_date() {
  $dates = ['foo', '200', '2000-01-01 23', '23:32 foo', "-x year"];

  foreach ($dates as $date) {
    try {
      $date = new DateTime($date);
      var_dump($date->format('Y-m-d'));
    } catch (Exception $e) {
     var_dump($e->getMessage());
     var_dump(DateTime::getLastErrors());
    }
  }
}


function test_constructor_relative_date() {
  $time = new \DateTime("-1 year");
  echo $time->format('Y/m/d'), "\n";
}

test_constructor_valid_date();
test_constructor_invalid_date();
test_constructor_relative_date();
