<?php

class BenchmarkJson {
  private static $array_data = [
    'a' => [
      'b' => [
        'c' => [
          'd' => 10,
          'e' => 20,
        ],
        'f' => 30,
      ],
      50 => [],
      60 => [],
      70 => [],
    ],
    'g' => [
      [1],
      [2],
      [3],
      [1, 2, 3],
      [1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
    ],
  ];

  public function benchmarkEncodeSafeArray() {
    return vk_json_encode_safe(self::$array_data);
  }
}
