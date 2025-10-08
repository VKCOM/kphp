@ok
<?php

function test_positive_numbers() {
  foreach ([0, 1, 2, 3, 10, 15, 37,
            100, 1000, 10000, 10000000, 2000000000,
            200000000000, 1212513212774, 684894187984361548,
            2147483647, 4294967295, 9223372036854775807
           ] as $n) {
    var_dump(decbin($n));
    var_dump(bindec(decbin($n)));

    var_dump(dechex($n));
    var_dump(hexdec(dechex($n)));
    echo "\n";
  }
}

function test_negative_numbers() {
  foreach ([-1, -2, -3, -10, -15, -37,
            -100, -1000, -10000, -10000000, -2000000000,
            -200000000000, -1212513212774, -684894187984361548,
            -2147483648, -4294967295, -9223372036854775807,
            -9223372036854775808
           ] as $n) {

    echo "\n";
    var_dump(decbin($n));
    var_dump(dechex($n));
#ifndef KPHP
    var_dump(is_float($n) ? (-PHP_INT_MAX - 1) : $n);
    var_dump(is_float($n) ? (-PHP_INT_MAX - 1) : $n);
    continue;
#endif
    var_dump(bindec(decbin($n)));
    var_dump(hexdec(dechex($n)));
  }
  echo "\n";
}

function test_bindec_overflow() {
#ifndef KPHP
  var_dump(-9223372036854775807 - 1);
  var_dump(-9223372036850581487);
  var_dump(108099585200619537);
  return;
#endif

  var_dump(bindec("1000000000000000000000000000000000000000000000000000000000000000"));
  var_dump(bindec("1000000000000000000000000000000000000000010000000000000000010001"));
  var_dump(bindec("10000000000110000000000011000000000000000000010000000000000000010001"));
}

function test_hexdec_overflow() {
#ifndef KPHP
  var_dump(-1);
  var_dump(-1152921504606846976);
  var_dump(-2305843006529339392);
  var_dump(-8070204238958949376);
  var_dump(-9223372036854775807 - 1);
  var_dump(18);
  return;
#endif

  var_dump(hexdec("ffffffffffffffff"));
  var_dump(hexdec("f000000000000000"));
  var_dump(hexdec("e0000000a0000000"));
  var_dump(hexdec("9000e000a0000c00"));
  var_dump(hexdec("8000000000000000"));
  var_dump(hexdec("800000000000000012"));
}

function test_bindec_bad_param() {
  var_dump(bindec("hello"));
  var_dump(bindec("0001h11ello"));
  var_dump(bindec("gg 11 001h11ello"));
  var_dump(bindec("dfm138u17h2-4120"));
}

function test_hexdec_bad_param() {
  var_dump(hexdec("hello"));
  var_dump(hexdec("0001h11ello"));
  var_dump(hexdec("gg 11 001h11ello"));
  var_dump(hexdec("dfm138u17h2-4120"));
}

test_positive_numbers();
test_negative_numbers();
test_bindec_overflow();
test_hexdec_overflow();
test_bindec_bad_param();
test_hexdec_bad_param();
