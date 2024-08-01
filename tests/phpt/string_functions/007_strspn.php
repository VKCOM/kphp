@ok php8 k2_skip
<?php

$haystacks = [
  '',
  "hell",
  "hello",
  "hello, world!",
];

$masks = [
  '',
  ' ',
  ' helo',
  '!?',
  '!',
  '?',
  "\n\t \r",
  "a",
  "h",
  "helo",
  'hhh',
  'hello',
];

$offsets = [
  0,
  1,
  10,
  -1,
  -2,
  -10,
  100,
  -100,
];

$all_haystacks = [];
foreach ($haystacks as $h) {
  $all_haystacks[] = $h;
  $all_haystacks[] = "$h$h";
  $all_haystacks[] = "PREFIX $h";
  $all_haystacks[] = "$h SUFFIX";
  $all_haystacks[] = "$h$h!!!!?";
  $all_haystacks[] = "$h$h!??";
  $all_haystacks[] = "  $h";
  $all_haystacks[] = "\t \t$h\t\t";
  $all_haystacks[] = "\n$h ";
  $all_haystacks[] = "\n\r$h ";
}

foreach ($all_haystacks as $haystack) {
  foreach ($masks as $char_list) {
    var_dump(strspn($haystack, $char_list));
    var_dump(strcspn($haystack, $char_list));
    foreach ($offsets as $offset) {
      var_dump(strspn($haystack, $char_list, $offset));
      var_dump(strcspn($haystack, $char_list, $offset));
    }
  }
}
