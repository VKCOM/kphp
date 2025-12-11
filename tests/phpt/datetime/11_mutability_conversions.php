@ok
<?php

function test_create_from_mutable() {
  $date = new DateTime("2014-06-20 11:45 Europe/London");
  $immutable = DateTimeImmutable::createFromMutable($date);

  echo $date->format('y-m-d h:i:s'), "\n";
  echo $immutable->format('y-m-d h:i:s'), "\n";

  $new_date = $date->setTimestamp(243415);
  $new_immutable = $immutable->setTimestamp(243415);

  echo $date->format('y-m-d h:i:s'), "\n";
  echo $new_date->format('y-m-d h:i:s'), "\n";
  echo $immutable->format('y-m-d h:i:s'), "\n";
  echo $new_immutable->format('y-m-d h:i:s'), "\n";
}

function test_create_from_immutable() {
  $date = new DateTimeImmutable("2014-06-20 11:45 Europe/London");
  $mutable = DateTime::createFromImmutable($date);

  echo $date->format('y-m-d h:i:s'), "\n";
  echo $mutable->format('y-m-d h:i:s'), "\n";

  $new_date = $date->setTimestamp(243415);
  $new_mutable = $mutable->setTimestamp(934234);

  echo $date->format('y-m-d h:i:s'), "\n";
  echo $new_date->format('y-m-d h:i:s'), "\n";
  echo $mutable->format('y-m-d h:i:s'), "\n";
  echo $new_mutable->format('y-m-d h:i:s'), "\n";
}

test_create_from_mutable();
test_create_from_immutable();
