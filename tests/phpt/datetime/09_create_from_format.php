@ok
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

function test_overflow_behaviour() {
  echo DateTime::createFromFormat('Y-m-d H:i:s', '2021-17-35 16:60:97')->format(DateTime::RFC2822), "\n";
  echo DateTime::createFromFormat(DateTime::RFC1123, 'Mon, 3 Aug 2020 25:00:00 +0000')->format(DateTime::RFC1123), "\n";
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

test_create_from_format();
test_create_from_format_errors();
test_overflow_behaviour();
test_create_from_format_tz_from_datetime();
test_create_from_format_timezone_param();
