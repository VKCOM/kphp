@ok
<?php

function test_add_date_interval(DateTimeInterface $date, DateInterval $interval) {
  $newDate = $date->add($interval);
  echo $date->format('Y-m-d H:i:s') . "\n";
  echo $newDate->format('Y-m-d H:i:s') . "\n";
}

function test_sub_date_interval(DateTimeInterface $date, DateInterval $interval) {
  $newDate = $date->sub($interval);
  echo $date->format('Y-m-d H:i:s') . "\n";
  echo $newDate->format('Y-m-d H:i:s') . "\n";
}

function test_subs_months() {
  $date = new DateTimeImmutable('2001-04-30');
  $interval = new DateInterval('P1M');

  $newDate1 = $date->sub($interval);
  echo $newDate1->format('Y-m-d') . "\n";

  $newDate2 = $newDate1->sub($interval);
  echo $newDate2->format('Y-m-d') . "\n";
}

test_add_date_interval(new DateTime('2000-01-01'), new DateInterval('P10D'));
test_add_date_interval(new DateTime('2000-01-01'), new DateInterval('PT10H30S'));
test_add_date_interval(new DateTime('2000-01-01'), new DateInterval('P7Y5M4DT4H3M2S'));

test_add_date_interval(new DateTimeImmutable('2000-01-01'), new DateInterval('P10D'));
test_add_date_interval(new DateTimeImmutable('2000-01-01'), new DateInterval('PT10H30S'));
test_add_date_interval(new DateTimeImmutable('2000-01-01'), new DateInterval('P7Y5M4DT4H3M2S'));

test_sub_date_interval(new DateTime('2000-01-20'), new DateInterval('P10D'));
test_sub_date_interval(new DateTime('2000-01-20'), new DateInterval('PT10H30S'));
test_sub_date_interval(new DateTime('2000-01-20'), new DateInterval('P7Y5M4DT4H3M2S'));

test_sub_date_interval(new DateTimeImmutable('2000-01-20'), new DateInterval('P10D'));
test_sub_date_interval(new DateTimeImmutable('2000-01-20'), new DateInterval('PT10H30S'));
test_sub_date_interval(new DateTimeImmutable('2000-01-20'), new DateInterval('P7Y5M4DT4H3M2S'));

test_subs_months();
