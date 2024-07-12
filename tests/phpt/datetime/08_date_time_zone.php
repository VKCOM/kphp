@ok k2_skip
<?php

function test_date_time_zone() {
  $d = new DateTimeZone('Europe/Moscow');
  echo $d->getName(), "\n";
  $d = new DateTimeZone('Etc/GMT-3');
  echo $d->getName(), "\n";

  try {
    $d = new DateTimeZone('Unknown');
    echo $d->getName(), "\n";
  } catch (Exception $e) {
   var_dump($e->getMessage());
   var_dump(DateTime::getLastErrors());
  }
}

test_date_time_zone();
