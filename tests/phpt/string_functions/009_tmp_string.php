@ok k2_skip
<?php

/**
 * @param string $s
 * @param ?string $optional_s
 * @param mixed $mixed
 * @param int $from
 * @param mixed $len
 */
function test_concat($s, $optional_s, $mixed, $from, $len) {
  var_dump($s . substr($s, $from, $len));
  var_dump($optional_s . substr($s, $from, $len));
  var_dump(substr($optional_s, $from, $len) . $s);
  var_dump(substr($optional_s, $from, $len) . $optional_s);
  var_dump(substr($mixed, $from) . substr($s, $from, $len));
  var_dump(substr($s, $from) . substr($s, $from, $len) . $mixed);

  var_dump($s . trim($s));
  var_dump($optional_s . trim($s));
  var_dump(trim($optional_s) . $s);
  var_dump(trim($optional_s) . $optional_s);
  var_dump(trim($mixed) . trim(substr($s, $from, $len)));
  var_dump(trim($mixed) . trim((string)substr($s, $from, $len)));
  var_dump(substr($s, $from) . trim($s) . $mixed);
}

/**
 * @param string $s
 * @param ?string $optional_s
 * @param mixed $mixed
 * @param int $from
 * @param mixed $len
 */
function test_conv_bool($s, $optional_s, $mixed, $from, $len) {
  var_dump((bool)substr($s, $from, $len));
  var_dump((bool)substr($optional_s, $from, $len));
  var_dump((bool)substr($mixed, $from, $len));
  var_dump((bool)substr(substr($s, 1, 3), $from, $len));
  var_dump((bool)substr($s, $from));
  var_dump((bool)substr($optional_s, $from));
  var_dump((bool)substr($mixed, $from));
  var_dump((bool)substr(substr($s, 1, 3), $from));
  var_dump((bool)substr((string)substr($s, 1, 3), $from));

  var_dump(substr($s, $from, $len) && $len);
  var_dump(substr($optional_s, $from, $len) && $len);
  var_dump(substr($mixed, $from, $len) && $len);
  var_dump(substr(substr($s, 1, 3), $from, $len) && $len);
  var_dump(substr($s, $from) && $len);
  var_dump(substr($optional_s, $from) && $len);
  var_dump(substr($mixed, $from) && $len);
  var_dump(substr(substr($s, 1, 3), $from) && $len);

  if (substr($s, $from, $len)) { var_dump(__LINE__); }
  if (substr($optional_s, $from, $len)) { var_dump(__LINE__); }
  if (substr($mixed, $from, $len)) { var_dump(__LINE__); }
  if (substr(substr($s, 1, 3), $from, $len)) { var_dump(__LINE__); }
  if (substr($s, $from)) { var_dump(__LINE__); }
  if (substr($optional_s, $from)) { var_dump(__LINE__); }
  if (substr($mixed, $from)) { var_dump(__LINE__); }
  if (substr(substr($s, 1, 3), $from)) { var_dump(__LINE__); }
}

/**
 * @param string $s
 * @param ?string $optional_s
 * @param mixed $mixed
 * @param int $from
 * @param mixed $len
 */
function test_index_substr($s, $optional_s, $mixed, $from, $len) {
  $map = [
    'foo' => 1,
    'hello, world' => 5.6,
    '432' => 'ok',
  ];
  /** @var mixed $map_as_mixed */
  $map_as_mixed = $map;

  var_dump($map[substr($s, $from, $len)]);
  var_dump($map[substr($s, $from)]);
  var_dump($map[substr($optional_s, $from, $len)]);
  var_dump($map[substr($optional_s, $from)]);
  var_dump($map[substr($mixed, $from, $len)]);
  var_dump($map[substr($mixed, $from)]);
  var_dump($map[(string)substr($mixed, $from)]);
  var_dump($map[substr($s, 1)]);
  var_dump($map[trim($s)]);

  var_dump($map_as_mixed[substr($s, $from, $len)]);
  var_dump($map_as_mixed[substr($s, $from)]);
  var_dump($map_as_mixed[substr($optional_s, $from, $len)]);
  var_dump($map_as_mixed[substr($optional_s, $from)]);
  var_dump($map_as_mixed[substr($mixed, $from, $len)]);
  var_dump($map_as_mixed[substr($mixed, $from)]);
  var_dump($map_as_mixed[substr($s, 1)]);
  var_dump($map_as_mixed[trim($s)]);

  var_dump($map[trim(substr($s, $from, $len))]);
  var_dump($map[trim(substr($s, $from))]);
  var_dump($map[trim(substr($optional_s, $from, $len))]);
  var_dump($map[trim(substr($optional_s, $from))]);

  $map[trim($s)] = 10334;
  var_dump($map[trim($s)]);
  $map[substr($optional_s, $from, $len)] = 342;
  var_dump($map[substr($optional_s, $from, $len)]);
  $map[substr($optional_s, $from, $len)] += 342;
  var_dump($map[substr($optional_s, $from, $len)]);

  $map_as_mixed[trim($s)] = 10334;
  var_dump($map_as_mixed[trim($s)]);
  $map_as_mixed[substr($optional_s, $from, $len)] = 342;
  var_dump($map_as_mixed[substr($optional_s, $from, $len)]);
  $map_as_mixed[substr($optional_s, $from, $len)] += 342;
  var_dump($map_as_mixed[substr($optional_s, $from, $len)]);

  /** @var string[] $map2 */
  $map2 = [
    (string)substr($s, $from, $len),
    (string)substr($s, $from),
    (string)substr($optional_s, $from, $len),
    (string)substr($optional_s, $from),
    (string)substr($mixed, $from, $len),
    (string)substr($mixed, $from),
    (string)substr(trim($s), $from, $len),
    (string)substr(trim($s), $from),
    (string)substr(trim($optional_s), $from, $len),
    (string)substr(trim($optional_s), $from),
    (string)substr(trim($mixed), $from, $len),
    (string)substr(trim($mixed), $from),
    trim(substr($s, $from, $len)),
    trim(substr($s, $from)),
    trim(substr($optional_s, $from, $len)),
    trim(substr($optional_s, $from)),
    trim(substr($mixed, $from, $len)),
    trim(substr($mixed, $from)),
    (string)substr($s, 1),
    trim($s),
  ];
  var_dump($map2);

  /** @var mixed $map3 */
  $map3 = [
    substr($s, $from, $len),
    substr($s, $from),
    substr($optional_s, $from, $len),
    substr($optional_s, $from),
    substr($mixed, $from, $len),
    substr($mixed, $from),
    substr(trim($s), $from, $len),
    substr(trim($s), $from),
    substr(trim($optional_s), $from, $len),
    substr(trim($optional_s), $from),
    substr(trim($mixed), $from, $len),
    substr(trim($mixed), $from),
    trim(substr($s, $from, $len)),
    trim(substr($s, $from)),
    trim(substr($optional_s, $from, $len)),
    trim(substr($optional_s, $from)),
    trim(substr($mixed, $from, $len)),
    trim(substr($mixed, $from)),
    substr($s, 1),
    trim($s),
  ];
  var_dump($map3);
}

/**
 * @param string $s
 * @param ?string $optional_s
 * @param mixed $mixed
 * @param int $from
 * @param mixed $len
 */
function test_intval_substr($s, $optional_s, $mixed, $from, $len) {
  var_dump("s='$s' intval=" . (int)substr($s, $from, $len));
  var_dump("s='$s' intval=" . (int)substr($optional_s, $from, $len));
  var_dump("s='$s' intval=" . (int)substr($mixed, $from, $len));
  var_dump("s='$s' intval=" . (int)substr(substr($s, 1, 3), $from, $len));
  var_dump("s='$s' intval=" . (int)substr($s, $from));
  var_dump("s='$s' intval=" . (int)substr($optional_s, $from));
  var_dump("s='$s' intval=" . (int)substr($mixed, $from));
  var_dump("s='$s' intval=" . (int)substr(substr($s, 1, 3), $from));

  var_dump("s='$s' intval=" . intval(substr($s, $from, $len)));
  var_dump("s='$s' intval=" . intval(substr($optional_s, $from, $len)));
  var_dump("s='$s' intval=" . intval(substr($mixed, $from, $len)));
  var_dump("s='$s' intval=" . intval(substr(substr($s, 1, 3), $from, $len)));
  var_dump("s='$s' intval=" . intval(substr($s, $from)));
  var_dump("s='$s' intval=" . intval(substr($optional_s, $from)));
  var_dump("s='$s' intval=" . intval(substr($mixed, $from)));
  var_dump("s='$s' intval=" . intval(substr(substr($s, 1, 3), $from)));

  var_dump("s='$s' intval=" . intval(trim($s)));
  var_dump("s='$s' intval=" . intval(trim($mixed)));
  var_dump("s='$s' intval=" . intval(trim(substr($s, 1))));
  var_dump("s='$s' intval=" . intval(substr(trim($s), 1)));

  /** @var int[] $arr */
  $arr = [];
  $arr[] = (int)substr(substr($s, 5, 10), 1);
  $arr[] = (int)substr(substr($mixed, 5, 10), 1);
  $arr[] = (int)substr((string)substr($s, 5, 10), 0, 2);
  $arr[] = (int)(string)substr((string)substr($mixed, 5, 10), 1, 10);
  var_dump($arr);

  /** @var mixed $arr_as_mixed */
  $arr_as_mixed = [];
  $arr_as_mixed[] = (int)substr(substr($s, 5, 10), 1);
  $arr_as_mixed[] = (int)substr(substr($mixed, 5, 10), 1);
  $arr_as_mixed[] = (int)substr((string)substr($s, 5, 10), 0, 2);
  $arr_as_mixed[] = (int)(string)substr((string)substr($mixed, 5, 10), 1, 10);
  var_dump($arr_as_mixed);
}

/**
 * @param string $s
 * @param ?string $optional_s
 * @param mixed $mixed
 * @param int $from
 * @param mixed $len
 */
function test_append_substr($s, $optional_s, $mixed, $from, $len) {
  $res = '';

  $res .= substr($s, $from, $len);
  var_dump($res);
  $res .= substr($optional_s, $from, $len);
  var_dump($res);
  $res .= substr($mixed, $from, $len);
  var_dump($res);
  $res .= substr(substr($s, 1, 3), $from, $len);
  var_dump($res);

  $res .= substr($s, $from);
  var_dump($res);
  $res .= substr($optional_s, $from);
  var_dump($res);
  $res .= substr($mixed, $from);
  var_dump($res);
  $res .= substr(substr($s, 1, 3), $from);
  var_dump($res);

  $res2 = 'x';
  $res2 .= substr(str_repeat($res2, 10), 10);
  var_dump($res2);
  $res2 .= substr($res2, 1);
  var_dump($res2);

  /** @var mixed $mixed2 */
  $mixed2 = '';
  $mixed2 .= substr($res2, 1);
  var_dump($mixed2);
  $mixed2 .= trim($res2);
  var_dump($mixed2);

  /** @var ?string $optional_s2 */
  $optional_s2 = '';
  $optional_s2 .= substr($res2, 1);
  var_dump($optional_s2);
  $optional_s2 .= substr($optional_s, 1);
  var_dump($optional_s2);
  $optional_s2 .= substr($optional_s2, 1);
  var_dump($optional_s2);

  $str_arr = [
    trim(substr($s, 1)) => 'a',
  ];
  var_dump($str_arr);
  $str_arr[trim(substr($s, 1))] .= 'b';
  var_dump($str_arr);
  $str_arr[trim(substr($s, 1))] = $str_arr[trim(substr($s, 1))] . 'c';
  var_dump($str_arr);
}


$strings = [
  '',
  ' ',
  'foo',
  'ab ab ab ab',
  'hello, world',
  '0',
  '1',
  '0432',
  '432',
  '-43',
  '+52',
  '93252',
  '3852881923',
  '234.63',
  '8438239.5234323',
];

$all_strings = [];
foreach ($strings as $s) {
  $all_strings[] = $s;
  $all_strings[] = "$s $s $s + $s";
  $all_strings[] = "-$s";
  $all_strings[] = "  +$s ";
  $all_strings[] = "$s    ";
  $all_strings[] = "    $s";
  $all_strings[] = "  -$s ";
}

$from_args = [
  0,
  1,
  2,
  10,
  -1,
  -10,
];

$len_args = [
  1,
  2,
  5,
  20,
  100,
  -2,
  -50,
];

foreach ($all_strings as $s) {
  foreach ($from_args as $from) {
    foreach ($len_args as $l) {
      test_intval_substr($s, $s, $s, $from, $l);
      test_intval_substr($s, null, $s, $from, $l);
      test_append_substr($s, $s, $s, $from, $l);
      test_append_substr($s, null, $s, $from, $l);
      test_index_substr($s, $s, $s, $from, $l);
      test_index_substr($s, null, $s, $from, $l);
      test_conv_bool($s, $s, $s, $from, $l);
      test_conv_bool($s, null, $s, $from, $l);
      test_concat($s, $s, $s, $from, $l);
      test_concat($s, null, $s, $from, $l);
    }
  }
}
