@ok k2_skip
<?php

/**
 * @param string $hint
 * @param (string|false|null)[] $arr
 * @param string|false|null $opt_string
 */
function test_array_functions($hint, $arr, $opt_string) {
  echo "array_key_exists $hint: "; var_dump(array_key_exists($opt_string, $arr));

  echo "array_search strict=true $hint: "; var_dump(array_search($opt_string, $arr, true));
  echo "array_search strict=false $hint: "; var_dump(array_search($opt_string, $arr, false));

  echo "in_array strict=true $hint: "; var_dump(in_array($opt_string, $arr, true));
  echo "in_array strict=false $hint: "; var_dump(in_array($opt_string, $arr, false));

  echo "array_fill $hint: "; var_dump(array_fill(-5, 17, $opt_string));

  echo "array_fill_keys $hint: "; var_dump(array_fill_keys($arr, $opt_string));

  echo "array_pad $hint: "; var_dump(array_pad($arr, 5, $opt_string));

  // TODO KPHP-485
  if (!is_null($opt_string)) {
    echo "array_column $hint: "; var_dump(array_column([$arr, $arr], $opt_string));
  }

  $res = array_push($arr, $opt_string);
  echo "array_push $hint: res = $res, "; var_dump($arr);

  $res = array_unshift($arr, $opt_string);
  echo "array_unshift $hint: res = $res, "; var_dump($arr);

}

$arrays = [
  [], // 0
  ["x" => "x", false => "f", null => "n", "" => "e", "false_value" => false, "null_value" => null], // 1
  ["1", null, false], // 2
  ["x" => "x", "" => "e"], // 3
  [null => null, false => false] // 4
];

$optional_strings = [
  "null" => null,
  "false" => false,
  "false_str" => "false",
  "null_str" => "null",
  "x" => "x",
  "1" => "1",
  "empty_str" => ""
];

for ($i = 0; $i < count($arrays); ++$i) {
  foreach ($optional_strings as $key_hint => $optional_string) {
    test_array_functions("array$i $key_hint", $arrays[$i], $optional_string);
  }
}
