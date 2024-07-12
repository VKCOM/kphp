@ok k2_skip
<?php

/**
 * @param string $hint
 * @param ((string|false|null)[]|false|null)[]|false|null $arr
 */
function test_serialization($hint, $arr) {
  echo "var_dump $hint: "; var_dump($arr);
  echo "json_encode $hint: "; var_dump(json_encode($arr));
  echo "serialize $hint: "; var_dump(serialize($arr));
}


$arrays = [
  false, // 0
  null, // 1
  [ // 2
    ["1", null, false],
    [null => null, false => false],
  ],
  [ // 3
    [],
    null,
    false
  ],
  [ // 4
    ["x" => "x", false => "f", null => "n", "" => "e", "false_value" => false, "null_value" => null],
    ["1", null, false],
    ["x" => "x", "" => "e"],
    [null => null, false => false]
  ]
];

for ($i = 0; $i < count($arrays); ++$i) {
  test_serialization("array$i", $arrays[$i]);
}
