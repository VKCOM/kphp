@ok
<?php

function test_array_concat_empty() {
  /** @var int[] */
  $arr = [1, 2, 3];

  /** @var int[] */
  $empty1 = [];

  /** @var int[] */
  $empty2 = [];

  $tmp1 = $arr + $empty1;
  $tmp2 = $empty1 + $arr;
  $tmp3 = $empty1 + $empty2;

  $arr[0] = -1;
  $empty1[0] = -2;
  $empty2[0] = -3;
  $tmp1[0] = -4;
  $tmp2[0] = -5;
  $tmp3[0] = -6;

  var_dump($arr);
  var_dump($empty1);
  var_dump($empty2);
  var_dump($tmp1);
  var_dump($tmp2);
  var_dump($tmp3);

  // ==========================================================================

  $map = ["one" => 1, "two" => 2, "three" => 3];

  /** @var int[] */
  $empty3 = [];

  $tmp4 = $map + $empty3;
  $tmp5 = $empty3 + $map;

  $map["one"] = -1;
  $tmp4["one"] = -2;
  $tmp5["one"] = -3;

  var_dump($map);
  var_dump($tmp4);
  var_dump($tmp5);
}

test_array_concat_empty();
