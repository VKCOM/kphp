<?php

function simple_function($a) {
  while(0);
}

function ackermann($m, $n) {
  if ($m == 0) return $n + 1;
  if ($n == 0) return ackermann($m - 1, 1);
  return ackermann($m - 1, ackermann($m, ($n - 1)));
}

function fibo($n) {
  return ($n < 2) ? 1 : fibo($n - 2) + fibo($n - 1);
}

function gen_random($n) {
  global $LAST;
  return ($n * ($LAST = ($LAST * IA + IC) % IM)) / IM;
}

function heapsort($n, &$ra) {
  $l = ($n >> 1) + 1;
  $ir = $n;

  while (1) {
    if ($l > 1) {
      $rra = $ra[--$l];
    } else {
      $rra = $ra[$ir];
      $ra[$ir] = $ra[1];
      if (--$ir == 1) {
        $ra[1] = $rra;
        return;
      }
    }
    $i = $l;
    $j = $l << 1;
    while ($j <= $ir) {
      if (($j < $ir) && ($ra[$j] < $ra[$j + 1])) {
        $j++;
      }
      if ($rra < $ra[$j]) {
        $ra[$i] = $ra[$j];
        $j += ($i = $j);
      } else {
        $j = $ir + 1;
      }
    }
    $ra[$i] = $rra;
  }
}

function mkmatrix($rows, $cols) {
  $count = 1;
  $mx = [];
  for ($i = 0; $i < $rows; $i++) {
    for ($j = 0; $j < $cols; $j++) {
      $mx[$i][$j] = $count++;
    }
  }
  return $mx;
}

function mmult($rows, $cols, $m1, $m2) {
  $m3 = [];
  for ($i = 0; $i < $rows; $i++) {
    for ($j = 0; $j < $cols; $j++) {
      $x = 0;
      for ($k = 0; $k < $cols; $k++) {
        $x += $m1[$i][$k] * $m2[$k][$j];
      }
      $m3[$i][$j] = $x;
    }
  }
  return $m3;
}

class BenchmarkBasic {
  function benchmarkSimple() {
    $a = 0;
    for ($i = 0; $i < 1000000; $i++) {
      $a++;
    }

    $thisisanotherlongname = 0;
    for ($thisisalongname = 0; $thisisalongname < 1000000; $thisisalongname++) {
      $thisisanotherlongname++;
    }
  }

  function benchmarkSimpleCall() {
    for ($i = 0; $i < 1000000; $i++) {
      strlen("hallo");
    }
  }

  function benchmarkSimpleuCall() {
    for ($i = 0; $i < 1000000; $i++) {
      simple_function("hallo");
    }
  }

  function benchmarkMandel() {
    $w1 = 50;
    $h1 = 150;
    $recen = -.45;
    $imcen = 0.0;
    $r = 0.7;
    $s = 0;
    $rec = 0;
    $imc = 0;
    $re = 0;
    $im = 0;
    $re2 = 0;
    $im2 = 0;
    $x = 0;
    $y = 0;
    $w2 = 0;
    $h2 = 0;
    $color = 0;
    $s = 2 * $r / $w1;
    $w2 = 40;
    $h2 = 12;
    for ($y = 0; $y <= $w1; $y = $y + 1) {
      $imc = $s * ($y - $h2) + $imcen;
      for ($x = 0; $x <= $h1; $x = $x + 1) {
        $rec = $s * ($x - $w2) + $recen;
        $re = $rec;
        $im = $imc;
        $color = 1000;
        $re2 = $re * $re;
        $im2 = $im * $im;
        while (((($re2 + $im2) < 1000000) && $color > 0)) {
          $im = $re * $im * 2 + $imc;
          $re = $re2 - $im2 + $rec;
          $re2 = $re * $re;
          $im2 = $im * $im;
          $color = $color - 1;
        }
        if ($color == 0) {
          print "_";
        } else {
          print "#";
        }
      }
      print "<br>";
    }
  }

  function benchmarkMandel2() {
    $b = " .:,;!/>)|&IH%*#";
    //float r, i, z, Z, t, c, C;
    for ($y = 30; printf("\n"), $C = $y * 0.1 - 1.5, $y--;) {
      for ($x = 0; $c = $x * 0.04 - 2, $z = 0, $Z = 0, $x++ < 75;) {
        for ($r = $c, $i = $C, $k = 0; $t = $z * $z - $Z * $Z + $r, $Z = 2 * $z * $Z + $i, $z = $t, $k < 5000; $k++)
          if ($z * $z + $Z * $Z > 500000) break;
        echo $b[$k % 16];
      }
    }
  }

  function benchmarkAckermann() {
    $n = 7;
    $r = ackermann(3, $n);
    print "ackermann(3,$n): $r\n";
  }

  function benchmarkAry() {
    $n = 50000;
    for ($i = 0; $i < $n; $i++) {
      $X[$i] = $i;
    }
    for ($i = $n - 1; $i >= 0; $i--) {
      $Y[$i] = $X[$i];
    }
    $last = $n - 1;
    print "$Y[$last]\n";
  }

  function benchmarkAry2() {
    $n = 50000;
    for ($i = 0; $i < $n;) {
      $X[$i] = $i;
      ++$i;
      $X[$i] = $i;
      ++$i;
      $X[$i] = $i;
      ++$i;
      $X[$i] = $i;
      ++$i;
      $X[$i] = $i;
      ++$i;

      $X[$i] = $i;
      ++$i;
      $X[$i] = $i;
      ++$i;
      $X[$i] = $i;
      ++$i;
      $X[$i] = $i;
      ++$i;
      $X[$i] = $i;
      ++$i;
    }
    for ($i = $n - 1; $i >= 0;) {
      $Y[$i] = $X[$i];
      --$i;
      $Y[$i] = $X[$i];
      --$i;
      $Y[$i] = $X[$i];
      --$i;
      $Y[$i] = $X[$i];
      --$i;
      $Y[$i] = $X[$i];
      --$i;

      $Y[$i] = $X[$i];
      --$i;
      $Y[$i] = $X[$i];
      --$i;
      $Y[$i] = $X[$i];
      --$i;
      $Y[$i] = $X[$i];
      --$i;
      $Y[$i] = $X[$i];
      --$i;
    }
    $last = $n - 1;
    print "$Y[$last]\n";
  }

  function benchmarkAry3() {
    $n = 2000;
    for ($i = 0; $i < $n; $i++) {
      $X[$i] = $i + 1;
      $Y[$i] = 0;
    }
    for ($k = 0; $k < 1000; $k++) {
      for ($i = $n - 1; $i >= 0; $i--) {
        $Y[$i] += $X[$i];
      }
    }
    $last = $n - 1;
    print "$Y[0] $Y[$last]\n";
  }

  function benchmarkFibo() {
    $n = 30;
    $r = fibo($n);
    print "$r\n";
  }

  function benchmarkHash1() {
    $n = 50000;
    for ($i = 1; $i <= $n; $i++) {
      $X[dechex($i)] = $i;
    }
    $c = 0;
    for ($i = $n; $i > 0; $i--) {
      if ($X[dechex($i)]) {
        $c++;
      }
    }
    print "$c\n";
  }

  function benchmarkHash2() {
    $n = 500;
    for ($i = 0; $i < $n; $i++) {
      $hash1["foo_$i"] = $i;
      $hash2["foo_$i"] = 0;
    }
    for ($i = $n; $i > 0; $i--) {
      foreach ($hash1 as $key => $value) $hash2[$key] += $value;
    }
    $first = "foo_0";
    $last = "foo_" . ($n - 1);
    print "$hash1[$first] $hash1[$last] $hash2[$first] $hash2[$last]\n";
  }

  function benchmarkHeapsort() {
    $n = 20000;
    global $LAST;

    define("IM", 139968);
    define("IA", 3877);
    define("IC", 29573);

    $LAST = 42;
    for ($i = 1; $i <= $n; $i++) {
      $ary[$i] = gen_random(1);
    }
    heapsort($n, $ary);
    printf("%.10f\n", $ary[$n]);
  }

  function benchmarkMatrix() {
    $n = 20;
    $SIZE = 30;
    $m1 = mkmatrix($SIZE, $SIZE);
    $m2 = mkmatrix($SIZE, $SIZE);
    while ($n--) {
      $mm = mmult($SIZE, $SIZE, $m1, $m2);
    }
    print "{$mm[0][0]} {$mm[2][3]} {$mm[3][2]} {$mm[4][4]}\n";
  }

  function benchmarkNestedLoop() {
    $n = 12;
    $x = 0;
    for ($a = 0; $a < $n; $a++)
      for ($b = 0; $b < $n; $b++)
        for ($c = 0; $c < $n; $c++)
          for ($d = 0; $d < $n; $d++)
            for ($e = 0; $e < $n; $e++)
              for ($f = 0; $f < $n; $f++)
                $x++;
    print "$x\n";
  }

  function benchmarkSieve() {
    $n = 30;
    $count = 0;
    while ($n-- > 0) {
      $count = 0;
      $flags = range(0, 8192);
      for ($i = 2; $i < 8193; $i++) {
        if ($flags[$i] > 0) {
          for ($k = $i + $i; $k <= 8192; $k += $i) {
            $flags[$k] = 0;
          }
          $count++;
        }
      }
    }
    print "Count: $count\n";
  }

  function benchmarkStrcat() {
    $n = 200000;
    $str = "";
    while ($n-- > 0) {
      $str .= "hello\n";
    }
    $len = strlen($str);
    print "$len\n";
  }
}
