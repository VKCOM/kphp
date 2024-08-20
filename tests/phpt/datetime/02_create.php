@ok k2_skip
<?php

function test_constructor_valid_date() {
  var_dump((new DateTime())->format('Y-m-d'));
  var_dump((new DateTime(''))->format('Y-m-d'));
  var_dump((new DateTime('now'))->format('Y-m-d'));
  var_dump((new DateTimeImmutable())->format('Y-m-d'));
  var_dump((new DateTimeImmutable(''))->format('Y-m-d'));
  var_dump((new DateTimeImmutable('now'))->format('Y-m-d'));

  $dates = ['2000-01-01', '20000101 23:32:12', '2000-01-01 23:32:12',
    '2000-01-01 233212', '2000-01-01 23:32', '2000-01-01 2332', '23:32:12', '23:32', '233212', '2332'];

  foreach ($dates as $date) {
    var_dump((new DateTime($date))->format("Y-m-d H:i:s"));
    var_dump(DateTime::getLastErrors());
    var_dump((new DateTimeImmutable($date))->format("Y-m-d H:i:s"));
    var_dump(DateTimeImmutable::getLastErrors());
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

function test_immutable_constructor_invalid_date() {
  $dates = ['foo', '200', '2000-01-01 23', '23:32 foo', "-x year"];

  foreach ($dates as $date) {
    try {
      $date = new DateTimeImmutable($date);
      var_dump($date->format('Y-m-d'));
    } catch (Exception $e) {
     var_dump($e->getMessage());
     var_dump(DateTimeImmutable::getLastErrors());
    }
  }
}

function test_constructor_relative_date() {
  $time = new \DateTime("-1 year");
  echo $time->format('Y/m/d'), "\n";
  $time2 = new \DateTimeImmutable("-1 year");
  echo $time2->format('Y/m/d'), "\n";
}

function test_constructor_tz_from_datetime() {
  $dates = ['2010-01-28T15:00:00', '2010-01-28T15:00:00 Europe/Amsterdam', '2010-01-28T15:00:00 Pacific/Nauru',
    '@946684800', '2010-01-28T15:00:00+0430', '2010-01-28T15:00:00 GMT-06:00', '2010-01-28T15:00:00 CEST',
    '2010-01-28T15:00:00 BST', '2010-01-28T15:00:00 ACDT'];

  foreach ($dates as $date) {
    var_dump((new DateTime($date))->format("Y-m-d H:i:s e I O P T Z"));
    var_dump(DateTime::getLastErrors());
    var_dump((new DateTimeImmutable($date))->format("Y-m-d H:i:s e I O P T Z"));
    var_dump(DateTimeImmutable::getLastErrors());
  }
}

function test_constructor_timezone_param() {
  var_dump((new DateTime('2022-08-24 16:45:00'))->format('Y-m-d H:i:s e I O P T Z U'));
  var_dump((new DateTime('2022-08-24 16:45:00', new DateTimeZone('Europe/Moscow')))->format('Y-m-d H:i:s e I O P T Z U'));
  var_dump((new DateTime('2022-08-24 16:45:00', new DateTimeZone('Etc/GMT-3')))->format('Y-m-d H:i:s e I O P T Z U'));
  var_dump((new DateTime('2022-08-24 16:45:00 Pacific/Nauru', new DateTimeZone('Etc/GMT-3')))->format('Y-m-d H:i:s e I O P T Z U'));
  var_dump((new DateTime('2022-08-24 16:45:00 BST', new DateTimeZone('Europe/Moscow')))->format('Y-m-d H:i:s e I O P T Z U'));
  var_dump((new DateTime('2022-08-24 16:45:00 GMT-06:00', new DateTimeZone('Europe/Moscow')))->format('Y-m-d H:i:s e I O P T Z U'));
  var_dump((new DateTime('@946684800', new DateTimeZone('Etc/GMT-3')))->format('Y-m-d H:i:s e I O P T Z U'));

  var_dump((new DateTimeImmutable('2022-08-24 16:45:00'))->format('Y-m-d H:i:s e I O P T Z U'));
  var_dump((new DateTimeImmutable('2022-08-24 16:45:00', new DateTimeZone('Europe/Moscow')))->format('Y-m-d H:i:s e I O P T Z U'));
  var_dump((new DateTimeImmutable('2022-08-24 16:45:00', new DateTimeZone('Etc/GMT-3')))->format('Y-m-d H:i:s e I O P T Z U'));
  var_dump((new DateTimeImmutable('2022-08-24 16:45:00 Pacific/Nauru', new DateTimeZone('Etc/GMT-3')))->format('Y-m-d H:i:s e I O P T Z U'));
  var_dump((new DateTimeImmutable('2022-08-24 16:45:00 BST', new DateTimeZone('Europe/Moscow')))->format('Y-m-d H:i:s e I O P T Z U'));
  var_dump((new DateTimeImmutable('2022-08-24 16:45:00 GMT-06:00', new DateTimeZone('Europe/Moscow')))->format('Y-m-d H:i:s e I O P T Z U'));
  var_dump((new DateTimeImmutable('@946684800', new DateTimeZone('Etc/GMT-3')))->format('Y-m-d H:i:s e I O P T Z U'));
}

test_constructor_valid_date();
test_constructor_invalid_date();
test_immutable_constructor_invalid_date();
test_constructor_relative_date();
test_constructor_tz_from_datetime();
test_constructor_timezone_param();
