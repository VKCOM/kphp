@ok
<?php

require_once 'kphp_tester_include.php';

/** @param mixed $v */
function print_value($v, ?CompileTimeLocation $loc = null) {
    $loc = CompileTimeLocation::calculate($loc);
    var_dump(["line=$loc->line" => $v]);
}

$strings = [
  '',
  ' ',
  'hello, world!',
  'some longer string without commas',
  'foo bar baz',
  'a,b,c,',
  ',a,b,',
  'a,b,c,d,e,f,g',
  ',',
  ',,',
];

$delimiters = [
  ' ',
  ',',
  ', ',
  'a',
  'foo',
  'bar',
];

$all_strings = [];
foreach ($strings as $s) {
  $all_strings[] = $s;
  $all_strings[] = "$s$s";
  $all_strings[] = "$s $s";
  $all_strings[] = "$s $s $s $s";
  $all_strings[] = "$s,$s";
  $all_strings[] = ",$s";
  $all_strings[] = "$s,";
  $all_strings[] = ",$s,";
  $all_strings[] = ", $s";
  $all_strings[] = "$s, ";
  $all_strings[] = ", $s, ";
}
foreach ($delimiters as $d) {
  $all_strings[] = $d;
  $all_strings[] = "$d $d";
}

function run_tests(string $s, string $d) {
  if ($d === '') {
    // empty delimiter is not valid
    return;
  }

  [$p1] = explode($d, $s);
  print_value([$p1]);
  [$p1] = explode($d, $s, 1);
  print_value([$p1]);
  print_value(explode($d, $s)[0]);
  print_value(explode($d, $s, 1)[0]);
  print_value(explode($d, $s, 2)[0]);
  for ($n = 1; $n <= 5; $n++) {
    [$p1] = explode($d, $s, $n);
    print_value([$p1]);
  }

  print_value((string)explode($d, $s)[1]);
  print_value((string)explode($d, $s)[2]);
  print_value((string)explode($d, $s)[3]);
  $k = 10;
  print_value((string)explode($d, $s)[$k]);
  $k = 1;
  print_value((string)explode($d, $s)[$k]);

  if (substr_count($s, $d) >= 2) {
    [1 => $p2] = explode($d, $s);
    print_value($p2);
    [0 => $p1] = explode($d, $s);
    print_value($p1);

    [$p1, $p2] = explode($d, $s);
    print_value([$p1, $p2]);
    [$p1, $p2] = explode($d, $s, 2);
    print_value([$p1, $p2]);

    [, $p2] = explode($d, $s);
    print_value([$p2]);
    [$p1, ] = explode($d, $s, 2);
    print_value([$p1]);

    print_value(explode($d, $s)[1]);
    print_value(explode($d, $s, 2)[1]);
    print_value(explode($d, $s, 3)[1]);
    for ($n = 2; $n <= 5; $n++) {
      [1 => $p2] = explode($d, $s, $n);
      print_value($p2);
      [0 => $p1] = explode($d, $s, $n);
      print_value($p1);

      [$p1, $p2] = explode($d, $s, $n);
      print_value([$p1, $p2]);
    }
  }
  if (substr_count($s, $d) >= 3) {
    [2 => $p3, 1 => $p2] = explode($d, $s);
    print_value([$p2, $p3]);
    [0 => $p1, 2 => $p3] = explode($d, $s);
    print_value([$p1, $p3]);

    [$p1, $p2, $p3] = explode($d, $s);
    print_value([$p1, $p2, $p3]);
    [$p1, $p2, $p3] = explode($d, $s, 3);
    print_value([$p1, $p2, $p3]);

    [$p1, , $p3] = explode($d, $s);
    print_value([$p1, $p3]);
    [, $p2, $p3] = explode($d, $s);
    print_value([$p2, $p3]);
    [, , $p3] = explode($d, $s);
    print_value([$p3]);

    print_value(explode($d, $s)[2]);
    print_value(explode($d, $s, 3)[2]);
    print_value(explode($d, $s, 4)[2]);
    for ($n = 3; $n <= 5; $n++) {
      [2 => $p3, 1 => $p2] = explode($d, $s, $n);
      print_value([$p2, $p3]);
      [0 => $p1, 2 => $p3] = explode($d, $s, $n);
      print_value([$p1, $p3]);

      [$p1, , $p3] = explode($d, $s, $n);
      print_value([$p1, $p3]);
      [, $p2, $p3] = explode($d, $s, $n);
      print_value([$p2, $p3]);
      [, , $p3] = explode($d, $s, $n);
      print_value([$p3]);

      [$p1, $p2, $p3] = explode($d, $s, $n);
      print_value([$p1, $p2, $p3]);
    }
  }
  if (substr_count($s, $d) >= 4) {
    [$p1, $p2, $p3, $p4] = explode($d, $s);
    print_value([$p1, $p2, $p3, $p4]);
    [$p1, $p2, $p3, $p4] = explode($d, $s, 4);
    print_value([$p1, $p2, $p3, $p4]);
    print_value(explode($d, $s)[3]);
    print_value(explode($d, $s, 4)[3]);
    print_value(explode($d, $s, 5)[3]);
    for ($n = 4; $n <= 5; $n++) {
      [$p1, $p2, $p3, $p4] = explode($d, $s, $n);
      print_value([$p1, $p2, $p3, $p4]);
    }
  }
  if (substr_count($s, $d) >= 5) {
    [$p1, $p2, $p3, $p4, $p5] = explode($d, $s);
    print_value([$p1, $p2, $p3, $p4, $p5]);
    [$p1, $p2, $p3, $p4, $p5] = explode($d, $s, 5);
    print_value([$p1, $p2, $p3, $p4, $p5]);
  }
}

/** @param string $s */
function ensure_string(string $s) {}

function extra_tests() {
  list($gid, , $iid, $oid) = explode('_', '24953_foo_24_958312');
  $res = $gid . '/' . $iid . ':' . $oid;
  print_value($res);

  $key = 'a b c d e f';
  [, , $x, $y] = explode(' ', $key);
  ensure_string($x);
  ensure_string($y);

  $x = 10;
  var_dump($x);
  $x2 = 20;
  var_dump($x2);

  $x = explode('-', 'a_0-b_1-c_2-d_3')[1];
  var_dump($x);
  $x2 = explode('_', explode('-', 'a_0-b_1-c_2-d_3')[1])[0];
  var_dump($x2);
}

class Example {
  private $prefixes = ['foo' => '123'];

  public function test() {
    $lambda = function(string $val) {
      return explode('_', $val)[0];
    };
    $res = $lambda('a_b');
    var_dump($res);

    $res2 = array_map(function($val) {
      return explode('_', $val)[0];
    }, ['a_b', 'c_d', '1_2']);
    var_dump($res2);

    /** @var mixed $values */
    $values = [
      'a,324',
      '_,-3',
      '1,2',
    ];
    $res3 = array_map(fn($extra) => (int)explode(',', $extra)[1], $values);
    var_dump($res3);

    $full_key = 'foo-bar';
    if ($this->prefixes[explode('-', $full_key)[0]]) {
      var_dump(explode('-', $full_key)[0]);
    }
  }
}

extra_tests();
$e = new Example();
$e->test();

foreach ($all_strings as $s) {
  foreach ($delimiters as $d) {
    run_tests($s, $d);
    run_tests($d, $s);
  }
}
