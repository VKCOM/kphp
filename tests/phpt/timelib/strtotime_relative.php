@ok
<?php

$timezones = [
  'Etc/GMT-3',
  'Europe/Moscow',
];

$tests = [
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

$base_time = 1040;

foreach ($timezones as $tz) {
  date_default_timezone_set($tz);
  foreach ($tests as $test) {
      var_dump(strtotime("+$test", $base_time));
      var_dump(strtotime("-$test", $base_time));
      var_dump(strtotime("$test", $base_time));
  }
}
