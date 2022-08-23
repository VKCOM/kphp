@ok
<?php

function test_set_date() {
  $date = new DateTime("2009-01-30 19:34:10");
  echo $date->format(DATE_RFC2822) . "\n";
  $new_date = $date->setDate(2008, 02, 01);
  echo $date->format(DATE_RFC2822) . "\n";
  echo $new_date->format(DATE_RFC2822) . "\n";
}

function test_date_exceed_range() {
  $date = new DateTime();
  
  $date->setDate(2001, 2, 28);
  echo $date->format('Y-m-d') . "\n";
  
  $date->setDate(2001, 2, 29);
  echo $date->format('Y-m-d') . "\n";
  
  $date->setDate(2001, 14, 3);
  echo $date->format('Y-m-d') . "\n";
}

test_set_date();
test_date_exceed_range();
