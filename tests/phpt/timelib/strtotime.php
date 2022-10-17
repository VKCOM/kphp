@ok
<?php

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
  'Jan-2006-15',
  '15-Feb-2009',
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
  'Feb 2010',
  '2010 Feb',
  '6.1.2009 13:00+01:00',
  '10 October 2018 19:30 pm',
  '15',
  '2009-02-15 15:16:17',
  '2009-02-15',
  '23h 15m 03s',
];

$time = 0;

$formats = [
  'j.n.Y H:iP',
  'j F Y G:i a',
  'Y-m-d',
  'j-M-Y',
  'Y-m-d H:i:s',
  'Y-m-!d H:i:s',
  '!d',
  'H\h i\m s\s',
  'H i s',
  'Hh im ss',
  'Y-m-d H:i:s (T)',
];

foreach ($timezones as $tz) {
  date_default_timezone_set($tz);
  foreach ($tests as $i => $test) {
    var_dump(["$tz:$i:strtotime('$test', $time)" => strtotime($test, $time)]);
    var_dump(["$tz:$i:date_parse('$test')" => date_parse($test)]);
    foreach ($formats as $fmt) {
      $date_parse_from_format_result = date_parse_from_format($fmt, $test);
      unset($date_parse_from_format_result['zone']);
      var_dump(["$tz:$i:date_parse_from_format('$fmt', '$test')" => $date_parse_from_format_result]);
    }
    $time += $i;
  }
}

function test_check_memory_leak() {
  foreach (['UTC', 'BST', 'Europe/Amsterdam'] as $tz) {
    $date = "2006:02:12 23:12:23 $tz";
    strtotime($date);
    date_parse($date);
    date_parse_from_format('Y-m-d H:i:s', $date);
  }
}

test_check_memory_leak();
