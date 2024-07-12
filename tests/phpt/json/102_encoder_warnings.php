@kphp_runtime_should_warn k2_skip
/Warning: /: Corner case in json conversion, \[\] could be easy transformed to \{\}/
/Warning: /\['nested'\]\[.\]: Corner case in json conversion, \[\] could be easy transformed to \{\}/
/Warning: /: strange double inf in function json_encode/
/Warning: /\[\.\]: strange double inf in function json_encode/
/Warning: /\[\.\]\[\.\]: strange double inf in function json_encode/
/Warning: /\['a'\]: strange double inf in function json_encode/
/Warning: /\['a'\]\['b'\]: strange double inf in function json_encode/
/Warning: /\['a'\]\['b'\]\['c'\]: strange double inf in function json_encode/
/Warning: /\['a'\]\[\.\]: strange double inf in function json_encode/
/Warning: /\[\.\]\['a'\]\[\.\]: strange double inf in function json_encode/
/Warning: /\[\.\]\[\.\]\['a'\]\['b'\]\[\.\]: strange double inf in function json_encode/
/Warning: /\[\.\]\[\.\]\['a'\]\['b'\]: strange double inf in function json_encode/
/Warning: /\['d'\]\['e'\]: strange double inf in function json_encode/
/Warning: /\['d'\]: strange double inf in function json_encode/
/Warning: /\['k1'\]\['k2'\]\['k3'\]\['k4'\]\['k5'\]\['k6'\]\['k7'\]\['k8'\]\.\.\.: strange double inf in function json_encode/
/Warning: /\['k1'\]\['k2'\]\['a'\]: strange double inf in function json_encode/
/Warning: /\['a'\]\[\.\]: strange double inf in function json_encode/
/Warning: /\['a'\]\[\.\]: strange double inf in function json_encode/
/Warning: /\['a'\]\[\.\]: strange double inf in function json_encode/
/Warning: /\['a'\]\[\.\]: strange double inf in function json_encode/
/Warning: /\[\'.'\]: strange double inf in function json_encode/
/Warning: /\[\'\[.\]'\]: strange double inf in function json_encode/
/Warning: /\['negative'\]\[\.\]: strange double inf in function json_encode/
/Warning: /: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\[\.\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\[\.\]\[\.\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\['a'\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\['a'\]\['b'\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\['a'\]\['b'\]\['c'\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\['a'\]\[\.\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\[\.\]\['a'\]\[\.\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\[\.\]\[\.\]\['a'\]\['b'\]\[\.\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\[\.\]\[\.\]\['a'\]\['b'\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\['d'\]\['e'\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\['d'\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\['k1'\]\['k2'\]\['k3'\]\['k4'\]\['k5'\]\['k6'\]\['k7'\]\['k8'\]\.\.\.: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\['k1'\]\['k2'\]\['a'\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\['a'\]\[\.\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\['a'\]\[\.\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\['a'\]\[\.\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\['a'\]\[\.\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\[\'.'\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\[\'\[.\]'\]: Not a valid utf-8 character at pos 1 in function json_encode/
/Warning: /\['negative'\]\[\.\]: Not a valid utf-8 character at pos 1 in function json_encode/
<?php

// TODO: rewrite patterns when substring matching is implemented

function test_inf() {
  $x = INF;

  vk_json_encode_safe($x);

  $vec1 = [$x];
  vk_json_encode_safe($vec1);

  $vec2 = [[$x]];
  vk_json_encode_safe($vec2);

  $map1 = ['a' => $x];
  vk_json_encode_safe($map1);

  $map2 = ['a' => ['b' => $x]];
  vk_json_encode_safe($map2);

  $map3 = ['a' => ['b' => ['c' => $x]]];
  vk_json_encode_safe($map3);

  $mixed1 = ['a' => [1.0, $x, 1.5]];
  vk_json_encode_safe($mixed1);

  $mixed2 = [['a' => [$x, 1.5]]];
  vk_json_encode_safe($mixed2);

  $mixed3 = [[[1.5], ['a' => ['b' => [$x]]]]];
  vk_json_encode_safe($mixed3);

  $mixed4 = [[[1.5], ['a' => ['b' => $x]]]];
  vk_json_encode_safe($mixed4);

  $mixed5 = [
    'a' => [1.5],
    'b' => [
      'c' => 1.6,
    ],
    'd' => [
      'e' => $x,
    ],
    'f' => 0.0,
  ];
  vk_json_encode_safe($mixed5);

  $mixed6 = [
    'b' => [
      'c' => 1.6,
    ],
    'd' => $x,
    'f' => 0.0,
  ];
  vk_json_encode_safe($mixed6);

  $overflow1 = [
    'k1' => [
      'k2' => [
        'k3' => [
          'k4' => [
            'k5' => [
              'k6' => [
                'k7' => [
                  'k8' => [
                    'k9' => [
                      'k10' => $x,
                    ],
                  ],
                ],
              ],
            ],
          ],
        ],
      ],
    ],
  ];
  vk_json_encode_safe($overflow1);

  $overflow2 = [
    'k1' => [
      'k2' => [
        'k3' => [
          'k4' => [
            'k5' => [
              'k6' => [
                'k7' => [
                  'k8' => [
                    'k9' => [
                      'k10' => 'ok',
                    ],
                  ],
                ],
              ],
            ],
          ],
        ],
        'a' => $x,
        'b' => 'ok',
      ],
    ],
  ];
  vk_json_encode_safe($overflow2);

  $big_index1 = ['a' => [9 => $x]];
  vk_json_encode_safe($big_index1);

  $big_index2 = ['a' => [0, 1, 2, 3, 4, 5, 6, 7, 8, $x]];
  vk_json_encode_safe($big_index2);

  $big_index3 = ['a' => [10 => $x]];
  vk_json_encode_safe($big_index3);

  $big_index4 = ['a' => [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, $x]];
  vk_json_encode_safe($big_index4);

  $dot1 = ['.' => $x];
  vk_json_encode_safe($dot1);

  $dot2 = ['[.]' => $x];
  vk_json_encode_safe($dot2);

  $negative_key = ['negative' => [-3 => $x]];
  vk_json_encode_safe($negative_key);

  $strange_key = ['strangekey' => ['50a' => $x]];
  vk_json_encode_safe($strange_key);
}

function test_string_encoding() {
  $x = "\xe1";

  json_encode($x);

  $vec1 = [$x];
  json_encode($vec1);

  $vec2 = [[$x]];
  json_encode($vec2);

  $map1 = ['a' => $x];
  json_encode($map1);

  $map2 = ['a' => ['b' => $x]];
  json_encode($map2);

  $map3 = ['a' => ['b' => ['c' => $x]]];
  json_encode($map3);

  $mixed1 = ['a' => [1.0, $x, 1.5]];
  json_encode($mixed1);

  $mixed2 = [['a' => [$x, 1.5]]];
  json_encode($mixed2);

  $mixed3 = [[[1.5], ['a' => ['b' => [$x]]]]];
  json_encode($mixed3);

  $mixed4 = [[[1.5], ['a' => ['b' => $x]]]];
  json_encode($mixed4);

  $mixed5 = [
    'a' => [1.5],
    'b' => [
      'c' => 1.6,
    ],
    'd' => [
      'e' => $x,
    ],
    'f' => 0.0,
  ];
  json_encode($mixed5);

  $mixed6 = [
    'b' => [
      'c' => 1.6,
    ],
    'd' => $x,
    'f' => 0.0,
  ];
  json_encode($mixed6);

  $overflow1 = [
    'k1' => [
      'k2' => [
        'k3' => [
          'k4' => [
            'k5' => [
              'k6' => [
                'k7' => [
                  'k8' => [
                    'k9' => [
                      'k10' => $x,
                    ],
                  ],
                ],
              ],
            ],
          ],
        ],
      ],
    ],
  ];
  json_encode($overflow1);

  $overflow2 = [
    'k1' => [
      'k2' => [
        'k3' => [
          'k4' => [
            'k5' => [
              'k6' => [
                'k7' => [
                  'k8' => [
                    'k9' => [
                      'k10' => 'ok',
                    ],
                  ],
                ],
              ],
            ],
          ],
        ],
        'a' => $x,
        'b' => 'ok',
      ],
    ],
  ];
  json_encode($overflow2);

  $big_index1 = ['a' => [9 => $x]];
  json_encode($big_index1);

  $big_index2 = ['a' => [0, 1, 2, 3, 4, 5, 6, 7, 8, $x]];
  json_encode($big_index2);

  $big_index3 = ['a' => [10 => $x]];
  json_encode($big_index3);

  $big_index4 = ['a' => [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, $x]];
  json_encode($big_index4);

  $dot1 = ['.' => $x];
  json_encode($dot1);

  $dot2 = ['[.]' => $x];
  json_encode($dot2);

  $negative_key = ['negative' => [-3 => $x]];
  json_encode($negative_key);

  $strange_key = ['strangekey' => ['50a' => $x]];
  json_encode($strange_key);
}

function test_corner_case() {
  $v = [];
  $v['1'] = 'a';
  unset($v['1']);
  json_encode($v);
  vk_json_encode_safe($v);

  $v2 = ['nested' => [10 => $v]];
  json_encode($v2);
  vk_json_encode_safe($v2);
}

test_inf();
test_string_encoding();
test_corner_case();
