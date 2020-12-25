@ok php7_4
<?php

// why PHP7.4: 't0222 t0222' gives different results on some 7.2 builds

$timezones = [
  'Etc/GMT-3',
  'Europe/Moscow',
];

$tests = [
  '202012151314',
  "2005-07-14 22:30:41",
  "2005-07-14 22:30:41 GMT",
  "@1121373041",
  "@1121373041 CEST",
  '',
  " \t\r\n000",
  'yesterday',
  '22:49:12',
  '22:49:12 bogusTZ',
  '22.49.12.42GMT',
  '22.49.12.42bogusTZ',
  't0222',
  't0222 t0222',
  '022233',
  '022233 bogusTZ',
  '2-3-2004',
  '2.3.2004',
  '20060212T23:12:23UTC',
  '20060212T23:12:23 bogusTZ',
  '2006167',
  'Jan-15-2006',
  '2006-Jan-15',
  '10/Oct/2000:13:55:36 +0100',
  '10/Oct/2000:13:55:36 +00100',
  '2006',
  '1986',
  'JAN',
  'January',
  '1 Monday December 2008',
  '2 Monday December 2008',
  '3 Monday December 2008',
  'first Monday December 2008',
  'second Monday December 2008',
  'third Monday December 2008',
  'mayy 2 2009',
  '19970523091528',
  '20001231185859',
  '20800410101010',
  'back of 7',
  'front of 7',
  'back of 19',
  'front of 19',
];

$time = 0;

foreach ($timezones as $tz) {
  date_default_timezone_set($tz);
  foreach ($tests as $i => $test) {
    var_dump(["$tz:$i:strtotime('$test', $time)" => strtotime($test, $time)]);
    $time += $i;
  }
}
