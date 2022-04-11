@ok
<?php

require_once __DIR__ . '/../../benchmarks/BenchmarkStringSwitch.php';

function testPerfectHash_1char_1(string $s) {
  $x = 0;
  switch ($s) {
  case 'a':
  case 'b':
  case 'c':
    return 1;
  case 'd':
    return 2;
  case 'e':
    return 3;
  case 'f':
    $x = 10;
    break;
  case "\n":
    return 1323131;
  default:
    return -1;
  }
  return $x;
}

function testPerfectHash_1char_2(string $s) {
  $x = 0;
  switch ($s) {
  case "\xff":
    return 0xff;
  case '!':
  case '@':
  case '#':
  case '$':
    return ord($s);
  case 'd':
  case 'e':
  case 'f':
    $x = 10 + ord($s) * 30;
    break;
  case 'x':
    return 335;
  case 'y':
  case 'z':
    break;
  case 'a':
  default:
    return -1;
  }
  return $x;
}


function testPerfectHash_1char_3(string $s) {
  $x = 0;
  switch ($s) {
  case '!':
  case '@':
  case '#':
  case '$':
    switch ($s) {
    case '!':
      return 1;
    case '@':
      return 2;
    case '#':
      return 300;
    }
    return -100;
  case 'd':
  case 'f':
    $x = 10 + strlen($s);
    break;
  case 'e':
  case 'x':
    return 335;
  case 'y':
    $x = 30211;
    break;
  case 'z':
    break;
  case 'a':
  default:
    return -1;
  }
  return $x + 1;
}

function testPerfectHash_1char_4(string $s) {
  $x = 0;
  switch ($s) {
  case '!':
  case '@':
  case '#':
  case '$':
    switch ($s) {
    case '!':
      return 1;
    case '@':
    case ' ':
      return 300;
    }
    return -100;
  case 'd':
  case 'f':
  case ' ':
    $x = 10 + strlen($s);
    break;
  case 'e':
  case 'x':
    return 335;
  case 'z':
  case 'y':
    $x = 3011;
    break;
  case 'a':
  default:
    return -1;
  }
  return $x + 1;
}

function testPerfectHash_1char_5(string $s) {
  $x = 0;
  switch ($s) {
  case '!':
  case '@':
  case '#':
  case '$':
    switch ($s) {
    case '!':
      $x += 432777;
      break 2;
    case '#':
    case ' ':
      return 300;
    }
    $x += 11113;
  case 'd':
  case 'f':
  case ' ':
    $x = 10 + strlen($s);
    break;
  case 'e':
  case 'x':
    return 335;
  case 'z':
  case 'y':
    switch ($s) {
    case 'y':
      $x += 11;
      break 2;
    }
  case 'a':
    $x++;
  default:
    return $x;
  }
  return $x + 1;
}

function testPerfectHash_short_1(string $s) {
  $x = 0;
  switch ($s) {
  case 'one':
  case 'two':
  case 'three':
    return 1;
  case 'a':
    return 2;
  case 'prefix':
    return 3;
  case "\r":
    $x = 10;
    break;
  default:
    return -1;
  }
  return $x;
}

function testPerfectHash_short_2(string $s) {
  $x = 0;
  switch ($s) {
  case 'one':
  case 'two':
    break;
  case 'three':
    return 1;
  case 'prefixed':
    return 2;
  case 'six':
    return 3;
  case "\1":
  case "\2":
  case "\r":
    $x = 10;
    break;
  default:
    return -1;
  }
  return $x;
}

function testPerfectHash_short_3(string $s) {
  $x = 0;
  switch ($s) {
  case 'Foo\\':
    return 1132;
  case 'one':
    $x++;
  case 'two':
    $x++;
  case 'three':
    $x++;
  case 'four':
    $x += 50;
  case 'five':
    switch ($s) {
    case 'one':
      return 11 + $x;
    case 'five':
      return 22 + $x;
    default:
      return $x + 1;
    }
  case "\1":
  case "\2":
  case "\r":
  default:
    return -1;
  }
  return $x - 4;
}

function testPerfectHash_short_4(string $s) {
  $x = 0;
  switch ($s) {
  case 'a':
  case 'b':
    $x += 100;
  default:
    switch ($s) {
    case 'a':
      $x++;
    case 'b':
      $x++;
    case 'c':
      return $x;
    case 'd':
      return 10000;
    default:
      switch ($s) {
      case 'one':
        return -1;
      case 'two':
        return -2;
      case 'three':
      case 'four':
        break;
      case 'five':
        $x += 134;
        break 2;
      case 'six':
        $x += 130;
        break 2;
      }
    }
  }
  return $x;
}

function testPerfectHash_short_5(string $s) {
  $x = 0;
  switch ($s) {
  case "\xff\xff":
    return 222;
  case "\xff\xff\xff":
    return 333;
  case "\r":
    $x = 10;
    break;
  default:
    return -1;
  }
  return $x;
}

function testPerfectHash_prefixed_1(string $s) {
  $x = 0;
  switch ($s) {
  case 'prefixed_one':
  case 'prefixed_two':
  case 'prefixed_three':
    return 1;
  case 'prefixed':
    return 3;
  case 'prefixed_six':
    $x += 101;
  default:
    return $x;
  }
}

function testPerfectHash_prefixed_2(string $s) {
  $x = 10;
  switch ($s) {
  case 'prefixed_one':
    return 1;
  case 'prefixed_two':
    $x++;
  case 'prefixed_three':
    return $x;
  case 'prefixed_':
    $x += 4;
  case 'prefixed':
    return 10 + $x;
  case 'prefixed_six':
    $x += 1000;
  default:
    return $x;
  }
}

function testPerfectHash_prefixed_3(string $s) {
  $x = 10;
  switch ($s) {
  case 'prefixed_one':
  case 'prefixed_two':
    switch ($s) {
    case 'prefixed_one':
      return 10;
    }
    $x -= 113;
  case 'prefixed_three':
    return $x;
  case 'prefixed':
  case 'prefixed_six':
    $x += 10013;
  default:
    return $x;
  }
}

function testPerfectHash_prefixed_4(string $s) {
  $x = 10;
  switch ($s) {
  case 'prefixed_one':
  case 'prefixed_two':
    switch ($s) {
    case 'prefixed_one':
      $x += 50;
      break;
    }
    $x -= 113;
  case 'prefixed_three':
    $x += 4;
    break;
  case 'prefixed':
  case 'prefixed_six':
    $x += 10013;
  }
  return $x;
}

function testPerfectHash_prefixed_5(string $s) {
  $x = 10;
  switch ($s) {
  case 'prefixed_one':
  case 'prefixed_two':
  case 'prefixed_three':
    switch ($s) {
    case 'prefixed_three':
      break;
    default:
      $x += 42;
    }
    break;
  case 'prefixed':
  case 'prefixed_six':
  default:
    switch ($s) {
    case 'prefixed':
      return 1011;
    case 'prefixed_six':
      return 101122;
    }
    $x++;
  }
  return $x;
}

function testPerfectHash_prefixed_6(string $s) {
  $x = 0;
  switch ($s) {
  case 'one':
  case 'two':
  case 'three':
    switch ($s) {
    case 'one':
      $x += 3;
      break 2;
    case 'two':
      $x += 643;
      break 2;
    }
    $x += 411;
  case 'four':
    $x += 4;
    break;
  case 'five':
    $x += 50;
  }
  return $x;
}

function testPerfectHash_prefixed_7(string $s) {
  switch ($s) {
  case 'Foo\\Bar\\A':
    return 1;
  case 'Foo\\Bar\\B':
    return 2;
  case 'Foo\\Bar\\C\\':
    return 3;
  }
  return 0;
}

function testNormalHash1(string $s) {
  $x = 0;
  switch ($s) {
  case 'some unexpected string':
  case 'another string value':
  case 'no common prefix here':
    switch ($s) {
    case 'no common prefix here':
      $x += 3;
      break 2;
    case 'another string value':
      $x += 643;
      break 2;
    }
    $x += 411;
  case 'shorter string':
    $x += 4;
    break;
  case 'five':
    $x += 50;
  }
  return $x;
}

function testNormalHash2(string $s) {
  $x = 10;
  switch ($s) {
  case 'some unexpected string':
    return 1;
  case 'another string value':
    $x++;
  case 'prefixed_three':
    return $x;
  case 'prefixed_':
    $x += 4;
  case 'prefixed':
    return 10 + $x;
  case 'no common prefix here':
    $x += 1000;
  default:
    return $x;
  }
}

function testMixedHash1(string $s) {
  $x = 1011;
  switch ($s) {
  case 'prefixed_but_still_too_long':
    return 11;
  case 'prefixed_that_is_even_longer_than_another':
    $x++;
  case 'prefixed_string':
    return $x;
  case 'prefixed_1':
  case 'prefixed_2':
  case 'prefixed_one':
  case 'prefixed_two':
    switch ($s) {
    case 'prefixed_1':
      return 233;
    case 'prefixed_2':
      return 253;
    case 'prefixed_one':
      return 293;
    case 'prefixed_two':
      $x += 294;
    }
  case 'prefixed_two1':
    $x += 42;
  }
  return $x;
}

function testMixedHash2(string $s) {
  $x = 1011;
  switch ($s) {
  case 'prefixed_1':
    return 233;
  case 'prefixed_2':
    return 253;
  case 'prefixed_one':
    return 293;
  case 'prefixed_two':
    $x += 294;
    break;
  default:
    switch ($s) {
    case 'prefixed_but_still_too_long':
      return 11;
    case 'prefixed_that_is_even_longer_than_another':
      $x++;
    case 'prefixed_string':
      return $x;
    case 'prefixed_two1':
      $x += 42;
    }
  }
  return $x;
}

function testMixedHash3(string $s) {
  $x = 0;
  switch ($s) {
  case '1':
  case '2':
  case '3':
    return 1;
  case 'd':
    return 2;
  case 'e':
    return 3;
  case 'f':
    $x = 10;
    break;
  default:
    return -1;
  }
  return $x;
}

function test() {
  $tests = [
    "Foo\\Bar\\Baz", "Foo\\Bar", "Foo\\Bar\\", "Foo\\", "Bar\\",
    "Foo\\Bar\\A", "Foo\\Bar\\B", "Foo\\Bar\\C\\", "Foo\\Bar\\C",

    'prefixed_but_still_too_long', 'prefixed_that_is_even_longer_than_another',
    'no common prefix here', 'another string value', 'some unexpected string',
    'one', 'two', 'three', 'four', 'five', 'six', 'seven',
    'prefixed', 'prefixed_', 'prefix',
    'prefixed_one', 'prefixed_two', 'prefixed_three', 'prefixed_four', 'prefixed_five', 'prefixed_six', 'prefixed_seven',
    'prefixed_one1', 'prefixed_two1', 'prefixed_three1', 'prefixed_four1', 'prefixed_five1', 'prefixed_six1', 'prefixed_seven1',
    'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'x', 'y', 'z',
    'ab', 'bb', 'cb', 'db', 'eb', 'fb', 'gb',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '10',
    'a0', 'a1', 'a2', 'a3', 'a4', 'a5',
    'a1', 'b1', 'c1', 'd1', 'e1', 'f1', 'g1',
    'long string one', 'long string two',
    'hello', 'helo', 'olleh', 'ollhe', 'lll',
    '',
    '?', '!', '@', '#', '$', '%', '^',
    "\1", "\2", "\3", "\4",
    "\n", "\r", "\v",
    "\xff", "\xfa", "\xff\xff", "\xff\xff\xff",
    "\0", "\0\0", "\0\0a", "a\0", "a\0\0", "\1\0", "\0\1",
    "prefixed_one\0", "\0prefixed_one", "prefixed_\0one",
  ];

  // Just in case we overlooked something in the test list,
  // generate some extra test cases for every one of them
  $all_tests = [];
  foreach ($tests as $s) {
    $all_tests[] = $s;
    $all_tests[] = $s . $s;
    $all_tests[] = $s . "\0" . $s;
    $all_tests[] = $s . "\0";
    $all_tests[] = "\0" . $s;
    $all_tests[] = $s . ' extra suffix';
    $all_tests[] = 'extra prefix ' . $s;
    $all_tests[] = ' ' . $s;
    $all_tests[] = $s . ' ';
    $all_tests[] = '+' . $s;
    $all_tests[] = '-' . $s;
    $all_tests[] = $s . "\\";
    $all_tests[] = "\\" . $s;
  }

  foreach ($all_tests as $i => $s) {
    var_dump([__LINE__ => "$s => " . testPerfectHash_1char_1($s)]);
    var_dump([__LINE__ => "$s => " . testPerfectHash_1char_2($s)]);
    var_dump([__LINE__ => "$s => " . testPerfectHash_1char_3($s)]);
    var_dump([__LINE__ => "$s => " . testPerfectHash_1char_4($s)]);
    var_dump([__LINE__ => "$s => " . testPerfectHash_1char_5($s)]);

    var_dump([__LINE__ => "$s => " . testPerfectHash_short_1($s)]);
    var_dump([__LINE__ => "$s => " . testPerfectHash_short_2($s)]);
    var_dump([__LINE__ => "$s => " . testPerfectHash_short_3($s)]);
    var_dump([__LINE__ => "$s => " . testPerfectHash_short_4($s)]);
    var_dump([__LINE__ => "$s => " . testPerfectHash_short_5($s)]);

    var_dump([__LINE__ => "$s => " . testPerfectHash_prefixed_1($s)]);
    var_dump([__LINE__ => "$s => " . testPerfectHash_prefixed_2($s)]);
    var_dump([__LINE__ => "$s => " . testPerfectHash_prefixed_3($s)]);
    var_dump([__LINE__ => "$s => " . testPerfectHash_prefixed_4($s)]);
    var_dump([__LINE__ => "$s => " . testPerfectHash_prefixed_5($s)]);
    var_dump([__LINE__ => "$s => " . testPerfectHash_prefixed_6($s)]);
    var_dump([__LINE__ => "$s => " . testPerfectHash_prefixed_7($s)]);

    var_dump([__LINE__ => "$s => " . testNormalHash1($s)]);
    var_dump([__LINE__ => "$s => " . testNormalHash2($s)]);

    var_dump([__LINE__ => "$s => " . testMixedHash1($s)]);
    var_dump([__LINE__ => "$s => " . testMixedHash2($s)]);
    var_dump([__LINE__ => "$s => " . testMixedHash3($s)]);
  }
}

function test_benchmarks() {
  $b = new BenchmarkStringSwitch();
  $results = [
    $b->benchmarkSwitchOnlyDefault(),
    $b->benchmarkSwitch1(),
    $b->benchmarkSwitch1Miss(),
    $b->benchmarkLexerComplex(),
    $b->benchmarkLexerComplexMiss(),
    $b->benchmarkSwitch8oneChar(),
    $b->benchmarkSwitch8oneCharMiss(),
    $b->benchmarkSwitch8oneCharNoDefault(),
    $b->benchmarkSwitch8oneCharNoDefaultMiss(),
    $b->benchmarkSwitch6perfectHash(),
    $b->benchmarkSwitch6perfectHashMiss(),
    $b->benchmarkSwitch6perfectHashNoDefault(),
    $b->benchmarkSwitch6perfectHashNoDefaultMiss(),
    $b->benchmarkSwitch12perfectHash(),
    $b->benchmarkSwitch12perfectHashMiss(),
    $b->benchmarkSwitch12perfectHashNoDefault(),
    $b->benchmarkSwitch12perfectHashNoDefaultMiss(),
    $b->benchmarkSwitch6perfectHashWithPrefix(),
    $b->benchmarkSwitch6perfectHashWithPrefixMiss(),
    $b->benchmarkSwitch6perfectHashWithPrefixNoDefault(),
    $b->benchmarkSwitch6perfectHashWithPrefixNoDefaultMiss(),
    $b->benchmarkSwitch12perfectHashWithPrefix(),
    $b->benchmarkSwitch12perfectHashWithPrefixMiss(),
    $b->benchmarkSwitch12perfectHashWithPrefixNoDefault(),
    $b->benchmarkSwitch12perfectHashWithPrefixNoDefaultMiss1(),
    $b->benchmarkSwitch12perfectHashWithPrefixNoDefaultMiss2(),
    $b->benchmarkSwitchCombined(),
  ];
  foreach ($results as $i => $x) {
    var_dump(["bench$i" => $x]);
  }
}

test();
test_benchmarks();
