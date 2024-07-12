@ok k2_skip
<?php

/**
 * @param string $str
 * @param string|false|null $opt_string1
 * @param string|false|null $opt_string2
 */
function test_string_functions($str, $opt_string1, $opt_string2) {
  echo "strtr: "; var_dump(strtr($str, [$opt_string1, $opt_string2]));

  $count = 0;
  echo "str_replace: "; var_dump(str_replace($str, $opt_string1, $opt_string2, $count));
  echo "str_replace count: "; var_dump($count);

  echo "strpos: "; var_dump(strpos($str, $opt_string1, (int)$opt_string2));
  echo "strrpos: "; var_dump(strrpos($str, $opt_string1, (int)$opt_string2));
  echo "strstr: "; var_dump(strstr($str, $opt_string1, (bool)$opt_string2));

  if ($opt_string1) { // todo KPHP-517
    echo "stripos: "; var_dump(stripos($str, $opt_string1, (int)$opt_string2));
    echo "stristr: "; var_dump(stristr($str, $opt_string1, (bool)$opt_string2));
    echo "strripos: "; var_dump(strripos($str, $opt_string1, (int)$opt_string2));
  }
}

$str_array = ["wow hello world", "736t null 1234", "foo bar baz false gaz taz", ""];
$opt_str_array = ["", "hello", false, "3", null, "bar"];

foreach ($str_array as $str) {
  foreach ($opt_str_array as $opt_string1) {
    foreach ($opt_str_array as $opt_string2) {
      test_string_functions($str, $opt_string1, $opt_string2);
    }
  }
}
