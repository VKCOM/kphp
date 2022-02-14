@ok php8
<?php

function wrong() {
  return "wrong";
}

var_dump(match (0) {
  null => wrong(),
  false => wrong(),
  0.0 => wrong(),
  [] => wrong(),
  '' => wrong(),
  0 => 'int',
});

function get_value2() {
  return 0;
}

var_dump(match (get_value2()) {
  null => wrong(),
  false => wrong(),
  0.0 => wrong(),
  [] => wrong(),
  '' => wrong(),
  0 => 'int',
  default => 'default',
});
