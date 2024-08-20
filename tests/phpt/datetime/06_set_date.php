@ok k2_skip
<?php

function test_set_date() {
  $date = new DateTime("2009-01-30 19:34:10");
  echo $date->format(DATE_RFC2822) . "\n";
  $new_date = $date->setDate(2008, 02, 01);
  echo $date->format(DATE_RFC2822) . "\n";
  echo $new_date->format(DATE_RFC2822) . "\n";
}

function test_set_date_immutable() {
  $date = new DateTime("2009-01-30 19:34:10");
  echo $date->format(DATE_RFC2822) . "\n";
  $new_date = $date->setDate(2008, 02, 01);
  echo $date->format(DATE_RFC2822) . "\n";
  echo $new_date->format(DATE_RFC2822) . "\n";
}

function test_set_date_through_polymorphic_call(DateTimeInterface $date) {
  echo $date->format(DATE_RFC2822) . "\n";
  $new_date = $date->setDate(2008, 02, 01);
  echo $date->format(DATE_RFC2822) . "\n";
  echo $new_date->format(DATE_RFC2822) . "\n";
}

function test_set_date_exceed_range() {
  $date = new DateTime();
  
  $date->setDate(2001, 2, 28);
  echo $date->format('Y-m-d') . "\n";
  
  $date->setDate(2001, 2, 29);
  echo $date->format('Y-m-d') . "\n";
  
  $date->setDate(2001, 14, 3);
  echo $date->format('Y-m-d') . "\n";
}

function test_setiso_date() {
  $date = new DateTime();

  $new_date = $date->setISODate(2008, 2);
  echo $date->format('Y-m-d') . "\n";
  echo $new_date->format('Y-m-d') . "\n";

  $new_date2 = $date->setISODate(2008, 2, 7);
  echo $date->format('Y-m-d') . "\n";
  echo $new_date2->format('Y-m-d') . "\n";
}

function test_setiso_date_immutable() {
  $date = new DateTimeImmutable();

  $new_date = $date->setISODate(2008, 2);
  echo $date->format('Y-m-d') . "\n";
  echo $new_date->format('Y-m-d') . "\n";

  $new_date2 = $date->setISODate(2008, 2, 7);
  echo $date->format('Y-m-d') . "\n";
  echo $new_date2->format('Y-m-d') . "\n";
}

function test_setiso_date_through_polymorphic_call(DateTimeInterface $date) {
  $new_date = $date->setISODate(2008, 2);
  echo $date->format('Y-m-d') . "\n";
  echo $new_date->format('Y-m-d') . "\n";

  $new_date2 = $date->setISODate(2008, 2, 7);
  echo $date->format('Y-m-d') . "\n";
  echo $new_date2->format('Y-m-d') . "\n";
}

function test_setiso_date_exceed_range() {
  $date = new DateTime();

  $date->setISODate(2008, 2, 7);
  echo $date->format('Y-m-d') . "\n";

  $date->setISODate(2008, 2, 8);
  echo $date->format('Y-m-d') . "\n";

  $date->setISODate(2008, 53, 7);
  echo $date->format('Y-m-d') . "\n";
}

function test_setiso_date_find_month() {
  $date = new DateTime();
  $newDate = $date->setISODate(2008, 14);
  echo $newDate->format('n'), "\n";
}

test_set_date();
test_set_date_immutable();
test_set_date_through_polymorphic_call(new DateTime("2009-01-30 19:34:10"));
test_set_date_through_polymorphic_call(new DateTimeImmutable("2009-01-30 19:34:10"));
test_set_date_exceed_range();
test_setiso_date();
test_setiso_date_immutable();
test_setiso_date_through_polymorphic_call(new DateTime);
test_setiso_date_through_polymorphic_call(new DateTimeImmutable);
test_setiso_date_exceed_range();
test_setiso_date_find_month();
