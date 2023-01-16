@ok php8
<?php

function wrong() {
  return "wrong";
}

// function test_int($char) {
//   return match ($char) {
//     0 => wrong(),
//     1 => wrong(),
//     2 => wrong(),
//     3 => wrong(),
//     4 => wrong(),
//     5 => wrong(),
//     6 => wrong(),
//     7 => wrong(),
//     8 => wrong(),
//     9 => wrong(),
//     default => 'Not matched',
//   };
// }

// foreach (range(0, 9) as $int) {
//   var_dump((string)$int);
//   var_dump(test_int((string)$int));
// }

function test_string($int) {
  return match ($int) {
    '0' => wrong(),
    '1' => wrong(),
    '2' => wrong(),
    '3' => wrong(),
    '4' => wrong(),
    '5' => wrong(),
    '6' => wrong(),
    '7' => wrong(),
    '8' => wrong(),
    '9' => wrong(),
    default => 'Not matched',
  };
}

foreach (range(0, 9) as $int) {
  var_dump($int);
  var_dump(test_string($int));
}
