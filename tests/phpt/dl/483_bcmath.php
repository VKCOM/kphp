@ok
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

test_bcmath_negative_zero();
