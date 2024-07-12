@ok k2_skip
<?php

function test_set_timestamp() {
  $date = new DateTime;
  $new_date = $date->setTimestamp(1171502725);
  var_dump($date->format('U = Y-m-d H:i:s'));
  var_dump($new_date->format('U = Y-m-d H:i:s'));
}

function test_set_timestamp_immutable() {
  $date = new DateTimeImmutable('@946684800');
  $new_date = $date->setTimestamp(1171502725);
  var_dump($date->format('U = Y-m-d H:i:s'));
  var_dump($new_date->format('U = Y-m-d H:i:s'));
}

function test_set_timestamp_through_polymorphic_call(DateTimeInterface $date) {
  $new_date = $date->setTimestamp(1171502725);
  var_dump($date->format('U = Y-m-d H:i:s'));
  var_dump($date->getTimestamp());
  var_dump($new_date->format('U = Y-m-d H:i:s'));
  var_dump($new_date->getTimestamp());

  $new_date = $date->setTimestamp(0);
  var_dump($date->format('U = Y-m-d H:i:s'));
  var_dump($date->getTimestamp());
  var_dump($new_date->format('U = Y-m-d H:i:s'));
  var_dump($new_date->getTimestamp());

  $new_date = $date->setTimestamp(PHP_INT_MAX);
  var_dump($date->format('U = Y-m-d H:i:s'));
  var_dump($date->getTimestamp());
  var_dump($new_date->format('U = Y-m-d H:i:s'));
  var_dump($new_date->getTimestamp());

  $new_date = $date->setTimestamp(PHP_INT_MIN);
  var_dump($date->format('U = Y-m-d H:i:s'));
  var_dump($date->getTimestamp());
  var_dump($new_date->format('U = Y-m-d H:i:s'));
  var_dump($new_date->getTimestamp());
}

function test_set_timestamp_return_object() {
  $date = new DateTime;
  var_dump($date->format('Y-m-d'));

  $new_date = $date->setTimestamp(1171502725);
  var_dump($new_date->format('U = Y-m-d H:i:s'));
  var_dump($new_date->getTimestamp());
  var_dump($date->format('U = Y-m-d H:i:s'));
  var_dump($date->getTimestamp());
}

test_set_timestamp();
test_set_timestamp_immutable();
test_set_timestamp_through_polymorphic_call(new DateTime('@946684800'));
test_set_timestamp_through_polymorphic_call(new DateTimeImmutable('@946684800'));
test_set_timestamp_return_object();
