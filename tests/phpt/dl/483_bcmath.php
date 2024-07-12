@ok k2_skip
<?php
  var_dump (bcadd("-0.09999999999999", "-0.09999999999999", 1));
  var_dump (bcadd("0.09999999999999", "0", 1));
  var_dump (bcadd("0.051", "", 1));
  var_dump (bcadd("", "0.049", 1));
  var_dump (bcadd("0.051", "0.049", 0));
  var_dump (bcadd("0.051", "0.049", 1));
  var_dump (bcadd("0.051", "0.049", 2));
  var_dump (bcadd("0.051", "0.049", 3));
  var_dump (bcadd("0.051", "0.049", 10));
  var_dump (bcadd("99.88", ".11", 1));

  var_dump (bcsub("", "100.11", 1));
  var_dump (bcsub("1000099.88", "", 1));
  var_dump (bcsub("1000099.88", "100.11", 0));
  var_dump (bcsub("1000099.88", "100.11", 1));
  var_dump (bcsub("1000099.88", "100.11", 10));
  var_dump (bcsub("-1000099.88", "100.11", 0));
  var_dump (bcsub("-1000099.88", "100.11", 1));
  var_dump (bcsub("-1000099.88", "100.11", 10));
  var_dump (bcsub("1000099.88", "-100.11", 0));
  var_dump (bcsub("1000099.88", "-100.11", 1));
  var_dump (bcsub("1000099.88", "-100.11", 10));
  var_dump (bcsub("-1000099.88", "-100.11", 0));
  var_dump (bcsub("-1000099.88", "-100.11", 1));
  var_dump (bcsub("-1000099.88", "-100.11", 10));

  var_dump (bcsub("1000099.88", "1000099.88", 3));
  var_dump (bcadd("645345345.88", "745544599.77", 3));

  var_dump (bcmul("", "", 3));
  var_dump (bcmul("645345345.88", "", 1));
  var_dump (bcmul("", "745544599.77", 1));
  var_dump (bcmul("645345345.88", "745544599.77", 0));
  var_dump (bcmul("645345345.88", "745544599.77", 1));
  var_dump (bcmul("-645345345.88", "745544599.77", 2));
  var_dump (bcmul("645345345.88", "745544599.77", 3));
  var_dump (bcmul("645345345.88", "745544599.77", 4));

  var_dump (bcmul("645345345.88", "0.01", 6));
  var_dump (bcmul("645345345.88", "0.001", 6));
  var_dump (bcmul("-645345345.88", "0", 6));

  var_dump (bccomp("", "0", 6));
  var_dump (bccomp("", "-0", 6));
  var_dump (bccomp("-0", "", 6));
  var_dump (bccomp("0", "", 6));
  var_dump (bccomp("", "1", 6));
  var_dump (bccomp("", "-1", 6));
  var_dump (bccomp("-1", "", 6));
  var_dump (bccomp("1", "", 6));
  var_dump (bccomp("-645345345.88", "0", 6));
  var_dump (bccomp("-0", "0", 6));
  var_dump (bccomp("-0", "0.0000001", 6));
  var_dump (bccomp("-0", "0.0000010", 6));
  var_dump (bccomp("1212", "1221.0000010", 6));
  var_dump (bccomp("1222", "1221.0000010", 6));

  var_dump (bccomp('1', '2'));
  var_dump (bccomp('1.00001', '1', 3));
  var_dump (bccomp('1.00001', '1', 5));

  var_dump (bcdiv('', '6.55957', 3));
//  var_dump (bcdiv('105', '', 3));
  var_dump (bcdiv('105', '6.55957', 3));
  var_dump (bcdiv('3', '2', 0));
  var_dump (bcdiv('-105', '-6.55957', 3));

  var_dump (bcdiv('-12342305.123123123123123123123', '-6.55957', 0));
  var_dump (bcdiv('-12342305.123123123123123123123', '-6.55957', 2));
  var_dump (bcdiv('-12342305.123123123123123123123', '-6.55957', 10));
  var_dump (bcdiv('-12342305.123123123123123123123', '-6.55957', 100));

  var_dump (bcdiv('-12342305.123', '-6.55957', 0));
  var_dump (bcdiv('-12342305.123', '-6.55957', 2));
  var_dump (bcdiv('-12342305.123', '-6.55957', 10));
  var_dump (bcdiv('-12342305.123', '-6.55957', 100));

  var_dump (bcdiv('-12342305.123', '-621341234234234124.55957', 0));
  var_dump (bcdiv('-12342305.123', '-61242341234234234234.55957', 2));
  var_dump (bcdiv('-12342305.123', '-6234231423423423412342.55957', 10));
  var_dump (bcdiv('-12342305.123', '-612432342342342342342342.55957', 100));

  var_dump (bcdiv('-12342305.122342343', '-621341234234234124.55957', 0));
  var_dump (bcdiv('-12342305.12234234233', '-61242341234234234234.55957', 2));
  var_dump (bcdiv('-12342305.123423423', '-6234231423423423412342.55957', 10));
  var_dump (bcdiv('-12342305.1234234233', '-612432342342342342342342.55957', 100));

  var_dump (bcdiv('0.000001', '0.001', 20));
  var_dump (bcdiv('0.000001', '0.000000001', 20));
  var_dump (bcdiv('1000001', '0.001', 20));
  var_dump (bcdiv('1000001', '0.000000001', 20));

  var_dump (bcdiv('0.000001', '0.001', 0));
  var_dump (bcdiv('0.000001', '0.000000001', 0));
  var_dump (bcdiv('1000001', '0.001', 0));
  var_dump (bcdiv('1000001', '0.000000001', 0));

  var_dump (bcdiv("645345345.88", "745599.77", 0));
  var_dump (bcdiv("645345345.88", "745599.77", 1));
  var_dump (bcdiv("-645345345.88", "745599.77", 2));
  var_dump (bcdiv("645345345.88", "745599.77", 3));
  var_dump (bcdiv("645345345.88", "745599.77", 4));

  var_dump (bcdiv("645345345.88", "0.01", 6));
  var_dump (bcdiv("645345345.88", "0.001", 6));
  var_dump (bcdiv("-645345345.88", "0.1", 6));

  $a = array ("0.0", "-1.111111", "1.12312312", "-1.111111", "1.12312312", "1.111111", "-1.12312312", "1000101010101", "9999999999999", "0000.1111111", "-123123213213.1", "-1");
  $b = array (2, 3, 4, 5, 10, 20, 100);
  foreach ($a as $u)
    foreach ($a as $v) 
      foreach ($b as $scale) {
        var_dump ("$u + $v = ".bcadd ($u, $v, $scale));
        var_dump ("$u - $v = ".bcsub ($u, $v, $scale));
        var_dump ("$u * $v = ".bcmul ($u, $v, $scale));
        if ($v != "0.0") {
          var_dump ("$u / $v = ".bcdiv ($u, $v, $scale));
        }
        var_dump ("$u <=> $v = ".bccomp ($u, $v, $scale));
      }

function test_bcmath_negative_zero() {
/*
  bcmath behaviour was fixed since 7.4.23 and 8.0.10 php versions.
  In new php versions and in kphp bcmath never returns numbers like -0, -0.0 (but returns 0, 0.0).
  See https://bugs.php.net/bug.php?id=78238 for more details.
*/
  #ifndef KPHP
  if ((version_compare(PHP_VERSION, "7.4.23") >= 0) &&
       (version_compare(PHP_VERSION, "8.0.0") < 0) ||
       (version_compare(PHP_VERSION, "8.0.10") >= 0)) {
  #endif
    var_dump(bcadd("0", "-0.1"));    // 0
    var_dump(bcadd("0", "-0.1", 1)); // -0.1
    var_dump(bcadd("0", "-0.1", 2)); // -0.10

    var_dump(bcmul("0.9", "-0.1"));    // 0
    var_dump(bcmul("0.9", "-0.1", 1)); // 0.0
    var_dump(bcmul("0.9", "-0.1", 2)); // -0.09

    var_dump(bcmul("0.90", "-0.1"));    // 0
    var_dump(bcmul("0.90", "-0.1", 1)); // 0.0
    var_dump(bcmul("0.90", "-0.1", 2)); // -0.09
  #ifndef KPHP
  } else {
    var_dump("0");
    var_dump("-0.1");
    var_dump("-0.10");

    var_dump("0");
    var_dump("0.0");
    var_dump("-0.09");

    var_dump("0");
    var_dump("0.0");
    var_dump("-0.09");
  }
  #endif
}

function test_bcsqrt($nums) {
  foreach ($nums as $num) {
    echo bcsqrt($num), "\n";

    for ($i = 0; $i < 30; ++$i) {
      echo "num: $num; scale: $i; sqrt: ", bcsqrt($num, $i), "\n";
    }
  }
}

function test_bcsqrt_scale($num) {
  echo bcsqrt($num), "\n";
  bcscale(1);
  echo bcsqrt($num), "\n";
  echo bcsqrt($num, 0), "\n";
  echo bcsqrt($num, 2), "\n";
  echo bcsqrt($num, 3), "\n";
  bcscale(2);
  echo bcsqrt($num), "\n";
  echo bcsqrt($num, 0), "\n";
  echo bcsqrt($num, 1), "\n";
  echo bcsqrt($num, 2), "\n";
  echo bcsqrt($num, 3), "\n";
  bcscale(3);
  echo bcsqrt($num), "\n";
  echo bcsqrt($num, 0), "\n";
  echo bcsqrt($num, 1), "\n";
  echo bcsqrt($num, 2), "\n";
  echo bcsqrt($num, 3), "\n";
  bcscale(4);
  echo bcsqrt($num), "\n";
  echo bcsqrt($num, 0), "\n";
  echo bcsqrt($num, 1), "\n";
  echo bcsqrt($num, 2), "\n";
  echo bcsqrt($num, 3), "\n";
  bcscale(0);
}

function test_bcmod($nums) {
  $count_outer = 0;

  foreach ($nums as $divident) {
    $divident = $count_outer++ % 2 ? -$divident : $divident;

    $count_inner = 0;

    foreach ($nums as $divisor) {
      if ($divisor == 0) {
        continue;
      }

      $divisor = $count_inner++ % 3 ? -$divisor : $divisor;
      echo bcmod($divident, $divisor), "\n";

      for ($i = 0; $i < 30; ++$i) {
        echo bcmod($divident, $divisor, $i), "\n";
      }
    }
  }
}

function test_bcpow($nums, $powers) {
  foreach ($nums as $base) {
    foreach ($powers as $power) {
      echo bcpow($base, $power), "\n";
      echo bcpow(-$base, $power), "\n";
      echo bcpow($base, -$power), "\n";
      echo bcpow(-$base, -$power), "\n";

      for ($i = 0; $i < 20; ++$i) {
        echo bcpow($base, $power, $i), "\n";
        echo bcpow(-$base, $power, $i), "\n";
        echo bcpow($base, -$power, $i), "\n";
        echo bcpow(-$base, -$power, $i), "\n";
      }
    }
  }
}

function gen_numbers($seed) {
  $res = [];
  foreach ([0.001, 0.01, 0.5, 3, 7, 10, 100, 1000, 10000, 100000, 1.0E+12] as $factor) {
    $res[] = max($seed * $factor, 0.0001);
  }
  return $res;
}

test_bcmath_negative_zero();

$low_nums = [-0, +0, 0, 0.12340463450737, 0.1234046, 0.123, 0.1, 0.0, 0., .0, 0.0001, 0.2, 0.2001, 0.000100000345, 00.01, 0.010305020100, 000.1000305020100];
$mid_nums = ["", 1, 1.0, 1.00, 1.01, 1.001, 2, 2.0, 2.00, 2.01, 2.001, 2.0001, 2.00010003, 3, 4, 5, 6, 7, 8, 9,
             9.12342135234234234, 9.12342135, 9.123, 9.1, 9.10, 9.100, 9.0, 9.00, 09.00, 009.00, 009.001, 9.00000001, 9.000000013445];
$big_nums = [23984732141234, 2134143131, 341341341.132441, 341341341.132441, 34134134.324132441, 34134134.0000441, 34134134.00000, 341341340.00000, 341341340.00012];
$math_constants = [M_PI, M_E, M_LOG2E, M_LOG10E, M_LN2, M_LN10, M_PI_2, M_PI_4, M_1_PI, M_2_PI, M_SQRTPI, M_2_SQRTPI, M_SQRT2, M_SQRT3, M_SQRT1_2, M_LNPI, M_EULER];

test_bcsqrt($low_nums);
test_bcsqrt($mid_nums);
test_bcsqrt($big_nums);
test_bcsqrt($math_constants);

test_bcsqrt(gen_numbers(0.2346));
test_bcsqrt(gen_numbers(0.7684576));
test_bcsqrt(gen_numbers(0.0913244));
test_bcsqrt(gen_numbers(0.0102004));
test_bcsqrt(gen_numbers(0.89305972));
test_bcsqrt(gen_numbers(M_PI));
test_bcsqrt(gen_numbers(M_E));
test_bcsqrt(gen_numbers(M_LOG2E));
test_bcsqrt(gen_numbers(M_SQRT2));
test_bcsqrt(gen_numbers(M_LNPI));
test_bcsqrt_scale(417.134576345);

test_bcmod($low_nums);
test_bcmod($mid_nums);
test_bcmod($big_nums);
test_bcmod($math_constants);

$powers = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 99, 100];
test_bcpow($low_nums, $powers);
test_bcpow($mid_nums, $powers);
