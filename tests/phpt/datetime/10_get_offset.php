@ok
<?php

function test_get_offset() {
  $date = new DateTime('2008-06-21');
  echo $date->getOffset(), "\n";

  $date = new DateTime('2008-06-21', new DateTimeZone('Europe/Moscow'));
  echo $date->getOffset(), "\n";

  $date = new DateTime('2008-06-21', new DateTimeZone('Etc/GMT-3'));
  echo $date->getOffset(), "\n";

  $date = new DateTime('2008-06-21 Pacific/Nauru');
  echo $date->getOffset(), "\n";

  $date = new DateTime('2008-06-21 BST');
  echo $date->getOffset(), "\n";

  $date = new DateTime('2008-06-21 GMT-06:00');
  echo $date->getOffset(), "\n";

  $date = new DateTime('@946684800');
  echo $date->getOffset(), "\n";

  $winter = new DateTime('2010-12-21 America/New_York');
  echo $winter->getOffset(), "\n";

  $summer = new DateTime('2008-06-21 America/New_York');
  echo $summer->getOffset(), "\n";
}

function test_get_offset_immutable() {
  $date = new DateTimeImmutable('2008-06-21');
  echo $date->getOffset(), "\n";

  $date = new DateTimeImmutable('2008-06-21', new DateTimeZone('Europe/Moscow'));
  echo $date->getOffset(), "\n";

  $date = new DateTimeImmutable('2008-06-21', new DateTimeZone('Etc/GMT-3'));
  echo $date->getOffset(), "\n";

  $date = new DateTimeImmutable('2008-06-21 Pacific/Nauru');
  echo $date->getOffset(), "\n";

  $date = new DateTimeImmutable('2008-06-21 BST');
  echo $date->getOffset(), "\n";

  $date = new DateTimeImmutable('2008-06-21 GMT-06:00');
  echo $date->getOffset(), "\n";

  $date = new DateTimeImmutable('@946684800');
  echo $date->getOffset(), "\n";

  $winter = new DateTimeImmutable('2010-12-21 America/New_York');
  echo $winter->getOffset(), "\n";

  $summer = new DateTimeImmutable('2008-06-21 America/New_York');
  echo $summer->getOffset(), "\n";
}

test_get_offset();
test_get_offset_immutable();
