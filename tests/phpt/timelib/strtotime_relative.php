@ok k2_skip
<?php

$timezones = [
  'Etc/GMT-3',
  'Europe/Moscow',
];

$relative_parts = [
  '80412 seconds',
  '86399 seconds',
  '86400 seconds',
  '86401 seconds',
  '112913 seconds',
  '134 hours',
  '167 hours',
  '168 hours',
  '169 hours',
  '183 hours',
  '178 days',
  '179 days',
  '180 days',
  '183 days',
  '184 days',
  '115 months',
  '119 months',
  '120 months',
  '121 months',
  '128 months',
  '24 years',
  '25 years',
  '26 years',
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

$base_time = 1040;

foreach ($timezones as $tz) {
  date_default_timezone_set($tz);
  foreach ($relative_parts as $relative) {
    foreach ($tests as $test) {
      $combined = [
        "+$relative",
        "-$relative",
        "  +$relative ",
        "  -$relative ",
        "$test +$relative",
        "$test -$relative",
        "+$relative $test",
        "-$relative $test",
        "$relative",
      ];
      foreach ($combined as $t) {
        var_dump(["strtotime('$t')" => strtotime($t, $base_time)]);
        var_dump(["date_parse('$t')" => date_parse($t)]);
      }
    }
  }
}
