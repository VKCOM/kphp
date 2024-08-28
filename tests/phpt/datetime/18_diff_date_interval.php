@ok k2_skip
<?php

function test_diff_date_interval(DateTimeInterface $origin, DateTimeInterface $target, bool $absolute = false) {
  $interval = $origin->diff($target, $absolute);
  echo $interval->format('%H:%I:%S (Full days: %R%a)'), "\n";
  echo $origin->format(DateTimeInterface::ISO8601), "\n";
  echo $target->format(DateTimeInterface::ISO8601), "\n";
}

test_diff_date_interval(new DateTime('2009-10-11'), new DateTime('2009-10-13'));
test_diff_date_interval(new DateTime('2009-10-11'), new DateTime('2009-10-13'), true);
test_diff_date_interval(new DateTime('2021-10-30 09:00:00 Europe/London'), new DateTime('2021-10-31 08:30:00 Europe/London'));
test_diff_date_interval(new DateTimeImmutable('2009-10-11'), new DateTimeImmutable('2009-10-13'));
test_diff_date_interval(new DateTimeImmutable('2009-10-11'), new DateTimeImmutable('2009-10-13'), true);
test_diff_date_interval(new DateTimeImmutable('2021-10-30 09:00:00 Europe/London'), new DateTimeImmutable('2021-10-31 08:30:00 Europe/London'));

test_diff_date_interval(new DateTime('2009-10-13'), new DateTime('2009-10-11'));
test_diff_date_interval(new DateTime('2009-10-13'), new DateTime('2009-10-11'), true);
test_diff_date_interval(new DateTimeImmutable('2009-10-13'), new DateTimeImmutable('2009-10-11'));
test_diff_date_interval(new DateTimeImmutable('2009-10-13'), new DateTimeImmutable('2009-10-11'), true);
