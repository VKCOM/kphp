@ok k2_skip
<?php

function test_create_from_format() {
  $format = 'Y-m-d';
  $date = DateTime::createFromFormat($format, '2009-02-15');
  echo "Format: $format; " . $date->format('Y-m-d') . "\n";

  $format = 'd-m-Y';
  $date = DateTime::createFromFormat($format, '15-02-2009');
  echo "Format: $format; " . $date->format('Y-m-d') . "\n";

  $format = 'Y-m-d H:i:s';
  $date = DateTime::createFromFormat($format, '2009-02-15 15:16:17');
  echo "Format: $format; " . $date->format('Y-m-d H:i:s') . "\n";

  $format = 'Y-m-!d H:i:s';
  $date = DateTime::createFromFormat($format, '2009-02-15 15:16:17');
  echo "Format: $format; " . $date->format('Y-m-d H:i:s') . "\n";

  $format = '!d';
  $date = DateTime::createFromFormat($format, '15');
  echo "Format: $format; " . $date->format('Y-m-d H:i:s') . "\n";

  $format = 'i';
  $date = DateTime::createFromFormat($format, '15');
  echo "Format: $format; " . $date->format('Y-m-d H:i:s') . "\n";

  $format = 'H\h i\m s\s';
  $date = DateTime::createFromFormat($format, '23h 15m 03s');
  echo "Format: $format; " . $date->format('H:i:s') . "\n";
}

function test_create_from_format_immutable() {
  $format = 'Y-m-d';
  $date = DateTimeImmutable::createFromFormat($format, '2009-02-15');
  echo "Format: $format; " . $date->format('Y-m-d') . "\n";

  $format = 'd-m-Y';
  $date = DateTimeImmutable::createFromFormat($format, '15-02-2009');
  echo "Format: $format; " . $date->format('Y-m-d') . "\n";

  $format = 'Y-m-d H:i:s';
  $date = DateTimeImmutable::createFromFormat($format, '2009-02-15 15:16:17');
  echo "Format: $format; " . $date->format('Y-m-d H:i:s') . "\n";

  $format = 'Y-m-!d H:i:s';
  $date = DateTimeImmutable::createFromFormat($format, '2009-02-15 15:16:17');
  echo "Format: $format; " . $date->format('Y-m-d H:i:s') . "\n";

  $format = '!d';
  $date = DateTimeImmutable::createFromFormat($format, '15');
  echo "Format: $format; " . $date->format('Y-m-d H:i:s') . "\n";

  $format = 'i';
  $date = DateTimeImmutable::createFromFormat($format, '15');
  echo "Format: $format; " . $date->format('Y-m-d H:i:s') . "\n";

  $format = 'H\h i\m s\s';
  $date = DateTimeImmutable::createFromFormat($format, '23h 15m 03s');
  echo "Format: $format; " . $date->format('H:i:s') . "\n";
}

function test_create_from_format_errors() {
  $date = DateTime::createFromFormat('j-M-Y', '');
  var_dump((bool)$date);
  var_dump(DateTime::getLastErrors());

  $date = DateTime::createFromFormat('', '15-Feb-2009');
  var_dump((bool)$date);
  var_dump(DateTime::getLastErrors());

  $date = DateTime::createFromFormat('j-M-Y H', '15-Feb-2009');
  var_dump((bool)$date);
  var_dump(DateTime::getLastErrors());

  $date = DateTime::createFromFormat('j-M-Y', '15-Feb-2009 23');
  var_dump((bool)$date);
  var_dump(DateTime::getLastErrors());

  $date = DateTime::createFromFormat('j-M-Y', '15-Feb-hello');
  var_dump((bool)$date);
  var_dump(DateTime::getLastErrors());

  $date = DateTime::createFromFormat('j-M-X', '15-Feb-2009');
  var_dump((bool)$date);
  var_dump(DateTime::getLastErrors());
}

function test_create_from_format_errors_immutable() {
  $date = DateTimeImmutable::createFromFormat('j-M-Y', '');
  var_dump((bool)$date);
  var_dump(DateTime::getLastErrors());

  $date = DateTimeImmutable::createFromFormat('', '15-Feb-2009');
  var_dump((bool)$date);
  var_dump(DateTime::getLastErrors());

  $date = DateTimeImmutable::createFromFormat('j-M-Y H', '15-Feb-2009');
  var_dump((bool)$date);
  var_dump(DateTime::getLastErrors());

  $date = DateTimeImmutable::createFromFormat('j-M-Y', '15-Feb-2009 23');
  var_dump((bool)$date);
  var_dump(DateTime::getLastErrors());

  $date = DateTimeImmutable::createFromFormat('j-M-Y', '15-Feb-hello');
  var_dump((bool)$date);
  var_dump(DateTime::getLastErrors());

  $date = DateTimeImmutable::createFromFormat('j-M-X', '15-Feb-2009');
  var_dump((bool)$date);
  var_dump(DateTime::getLastErrors());
}

function test_overflow_behaviour() {
  echo DateTime::createFromFormat('Y-m-d H:i:s', '2021-17-35 16:60:97')->format(DateTime::RFC2822), "\n";
  echo DateTime::createFromFormat(DateTime::RFC1123, 'Mon, 3 Aug 2020 25:00:00 +0000')->format(DateTime::RFC1123), "\n";
  echo DateTimeImmutable::createFromFormat('Y-m-d H:i:s', '2021-17-35 16:60:97')->format(DateTime::RFC2822), "\n";
  echo DateTimeImmutable::createFromFormat(DateTime::RFC1123, 'Mon, 3 Aug 2020 25:00:00 +0000')->format(DateTime::RFC1123), "\n";
}

function test_create_from_format_tz_from_datetime() {
  echo (DateTime::createFromFormat('Y-m-d H:i:s', '2010-01-28 15:25:10'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";
  echo (DateTime::createFromFormat('Y-m-d H:i:s e', '2010-01-28 15:25:10 Europe/Amsterdam'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";
  echo (DateTime::createFromFormat('Y-m-d H:i:s e', '2010-01-28 15:25:10 Pacific/Nauru'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";
  echo (DateTime::createFromFormat('U', '946684800'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";
  echo (DateTime::createFromFormat('Y-m-d H:i:s e', '2010-01-28 15:25:10+0430'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";
  echo (DateTime::createFromFormat('Y-m-d H:i:s e', '2010-01-28 15:25:10 GMT-06:00'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";
  echo (DateTime::createFromFormat('Y-m-d H:i:s e', '2010-01-28 15:25:10 CEST'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";
  echo (DateTime::createFromFormat('Y-m-d H:i:s e', '2010-01-28 15:25:10 BST'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";
  echo (DateTime::createFromFormat('Y-m-d H:i:s e', '2010-01-28 15:25:10 ACDT'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";

  echo (DateTimeImmutable::createFromFormat('Y-m-d H:i:s', '2010-01-28 15:25:10'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";
  echo (DateTimeImmutable::createFromFormat('Y-m-d H:i:s e', '2010-01-28 15:25:10 Europe/Amsterdam'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";
  echo (DateTimeImmutable::createFromFormat('Y-m-d H:i:s e', '2010-01-28 15:25:10 Pacific/Nauru'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";
  echo (DateTimeImmutable::createFromFormat('U', '946684800'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";
  echo (DateTimeImmutable::createFromFormat('Y-m-d H:i:s e', '2010-01-28 15:25:10+0430'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";
  echo (DateTimeImmutable::createFromFormat('Y-m-d H:i:s e', '2010-01-28 15:25:10 GMT-06:00'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";
  echo (DateTimeImmutable::createFromFormat('Y-m-d H:i:s e', '2010-01-28 15:25:10 CEST'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";
  echo (DateTimeImmutable::createFromFormat('Y-m-d H:i:s e', '2010-01-28 15:25:10 BST'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";
  echo (DateTimeImmutable::createFromFormat('Y-m-d H:i:s e', '2010-01-28 15:25:10 ACDT'))->format('Y-m-d H:i:s e I O P T Z U'), "\n";
}

function test_create_from_format_timezone_param() {
  $date = DateTime::createFromFormat('Y-m-d H:i:s', '2022-08-24 16:45:00');
  echo $date->format('Y-m-d H:i:s e I O P T Z U'), "\n";

  $date = DateTime::createFromFormat('Y-m-d H:i:s', '2022-08-24 16:45:00', new DateTimeZone('Europe/Moscow'));
  echo $date->format('Y-m-d H:i:s e I O P T Z U'), "\n";

  $date = DateTime::createFromFormat('Y-m-d H:i:s', '2022-08-24 16:45:00', new DateTimeZone('Etc/GMT-3'));
  echo $date->format('Y-m-d H:i:s e I O P T Z U'), "\n";

  $date = DateTime::createFromFormat('Y-m-d H:i:s e', '2022-08-24 16:45:00 Pacific/Nauru', new DateTimeZone('Etc/GMT-3'));
  echo $date->format('Y-m-d H:i:s e I O P T Z U'), "\n";

  $date = DateTime::createFromFormat('Y-m-d H:i:s e', '2022-08-24 16:45:00 BST', new DateTimeZone('Europe/Moscow'));
  echo $date->format('Y-m-d H:i:s e I O P T Z U'), "\n";

  $date = DateTime::createFromFormat('Y-m-d H:i:s e', '2022-08-24 16:45:00 GMT-06:00', new DateTimeZone('Europe/Moscow'));
  echo $date->format('Y-m-d H:i:s e I O P T Z U'), "\n";

  $date = DateTime::createFromFormat('U', '946684800', new DateTimeZone('Etc/GMT-3'));
  echo $date->format('Y-m-d H:i:s e I O P T Z U'), "\n";
}

function test_create_from_format_timezone_param_immutable() {
  $date = DateTimeImmutable::createFromFormat('Y-m-d H:i:s', '2022-08-24 16:45:00');
  echo $date->format('Y-m-d H:i:s e I O P T Z U'), "\n";

  $date = DateTimeImmutable::createFromFormat('Y-m-d H:i:s', '2022-08-24 16:45:00', new DateTimeZone('Europe/Moscow'));
  echo $date->format('Y-m-d H:i:s e I O P T Z U'), "\n";

  $date = DateTimeImmutable::createFromFormat('Y-m-d H:i:s', '2022-08-24 16:45:00', new DateTimeZone('Etc/GMT-3'));
  echo $date->format('Y-m-d H:i:s e I O P T Z U'), "\n";

  $date = DateTimeImmutable::createFromFormat('Y-m-d H:i:s e', '2022-08-24 16:45:00 Pacific/Nauru', new DateTimeZone('Etc/GMT-3'));
  echo $date->format('Y-m-d H:i:s e I O P T Z U'), "\n";

  $date = DateTimeImmutable::createFromFormat('Y-m-d H:i:s e', '2022-08-24 16:45:00 BST', new DateTimeZone('Europe/Moscow'));
  echo $date->format('Y-m-d H:i:s e I O P T Z U'), "\n";

  $date = DateTimeImmutable::createFromFormat('Y-m-d H:i:s e', '2022-08-24 16:45:00 GMT-06:00', new DateTimeZone('Europe/Moscow'));
  echo $date->format('Y-m-d H:i:s e I O P T Z U'), "\n";

  $date = DateTimeImmutable::createFromFormat('U', '946684800', new DateTimeZone('Etc/GMT-3'));
  echo $date->format('Y-m-d H:i:s e I O P T Z U'), "\n";
}

function test_implicit_current_time() {
  // the $date has to contain current H-i-s, despite it's not provided
  $date = DateTime::createFromFormat('Y-m-d', '2012-10-17');

  $hours = (int)$date->format('H');
  $minutes = (int)$date->format('i');
  $seconds = (int)$date->format('s');

  $sum = $hours + $minutes + $seconds;
  var_dump($sum > 0);
}

function test_implicit_current_time_immutable() {
  // the $date has to contain current H-i-s, despite it's not provided
  $date = DateTimeImmutable::createFromFormat('Y-m-d', '2012-10-17');

  $hours = (int)$date->format('H');
  $minutes = (int)$date->format('i');
  $seconds = (int)$date->format('s');

  $sum = $hours + $minutes + $seconds;
  var_dump($sum > 0);
}

test_create_from_format();
test_create_from_format_immutable();
test_create_from_format_errors_immutable();
test_create_from_format_errors();
test_overflow_behaviour();
test_create_from_format_tz_from_datetime();
test_create_from_format_timezone_param();
test_create_from_format_timezone_param_immutable();
test_implicit_current_time_immutable();
test_implicit_current_time();
