@ok
<?php
  var_dump (array ("result" => array ("result" => 1, "_" => "resultTrue"), "_" => "_") === array ("result" => array ("result" => 2, "_" => "resultTrue"), "_" => "_"));

  error_reporting (0);
  var_dump (error_reporting());

  $a = 21;
  $b = "21";
  $c = 21.0;
  $d = "1";
  $e = "199";
  $f = "";
  $g = true;
  $h = false;
  $i = null;
  $j = array ();
  $k = "21 apple";
  $l = array (0);
  $m = 0;
  $n = "0";

/*
  $a = "0xa";
  $b = "1e1";
  $c = "     10e-0";
  $d = "10e+0";
  $e = "   1e1";
  $f = "1e10";
  $g = "10000000000";
  $h = "123456789012345678901234567890";
  $i = "123456789012345678901234567891";
  $j = "0";
  $k = "-0";
  $l = 10;
  $m = ".1e2";
  $n = "";
*/

/*  if (0) {
    $a = false;
    $b = false;
    $c = false;
    $d = false;
    $e = false;
    $f = false;
    $g = false;
    $h = false;
    $i = false;
    $j = false;
    $k = false;
    $l = false;
    $m = false;
  }*/

  $arr = array ($a, $b, $c, $d, $e, $f, $g, $h, $i, $j, $k, $l, $m, $n);

  foreach ($arr as $o) {
     echo "line:".__LINE__." " . "$o == $a: "; var_dump ($o == $a);
     echo "line:".__LINE__." " . "$o == $b: "; var_dump ($o == $b);
     echo "line:".__LINE__." " . "$o == $c: "; var_dump ($o == $c);
     echo "line:".__LINE__." " . "$o == $d: "; var_dump ($o == $d);
     echo "line:".__LINE__." " . "$o == $e: "; var_dump ($o == $e);
     echo "line:".__LINE__." " . "$o == $f: "; var_dump ($o == $f);
     echo "line:".__LINE__." " . "$o == $g: "; var_dump ($o == $g);
     echo "line:".__LINE__." " . "$o == $h: "; var_dump ($o == $h);
     echo "line:".__LINE__." " . "$o == $i: "; var_dump ($o == $i);
     echo "line:".__LINE__." " . "$o == $j: "; var_dump ($o == $j);
     echo "line:".__LINE__." " . "$o == $k: "; var_dump ($o == $k);
     echo "line:".__LINE__." " . "$o == $l: "; var_dump ($o == $l);
     echo "line:".__LINE__." " . "$o == $m: "; var_dump ($o == $m);
     echo "line:".__LINE__." " . "$o == $n: "; var_dump ($o == $n);
     echo "line:".__LINE__." " . "$a == $o: "; var_dump ($a == $o);
     echo "line:".__LINE__." " . "$b == $o: "; var_dump ($b == $o);
     echo "line:".__LINE__." " . "$c == $o: "; var_dump ($c == $o);
     echo "line:".__LINE__." " . "$d == $o: "; var_dump ($d == $o);
     echo "line:".__LINE__." " . "$e == $o: "; var_dump ($e == $o);
     echo "line:".__LINE__." " . "$f == $o: "; var_dump ($f == $o);
     echo "line:".__LINE__." " . "$g == $o: "; var_dump ($g == $o);
     echo "line:".__LINE__." " . "$h == $o: "; var_dump ($h == $o);
     echo "line:".__LINE__." " . "$i == $o: "; var_dump ($i == $o);
     echo "line:".__LINE__." " . "$j == $o: "; var_dump ($j == $o);
     echo "line:".__LINE__." " . "$k == $o: "; var_dump ($k == $o);
     echo "line:".__LINE__." " . "$l == $o: "; var_dump ($l == $o);
     echo "line:".__LINE__." " . "$m == $o: "; var_dump ($m == $o);
     echo "line:".__LINE__." " . "$n == $o: "; var_dump ($n == $o);
  }

   echo "line:".__LINE__." " . "$a == $a: "; var_dump ($a == $a);
   echo "line:".__LINE__." " . "$a == $b: "; var_dump ($a == $b);
   echo "line:".__LINE__." " . "$a == $c: "; var_dump ($a == $c);
   echo "line:".__LINE__." " . "$a == $d: "; var_dump ($a == $d);
   echo "line:".__LINE__." " . "$a == $e: "; var_dump ($a == $e);
   echo "line:".__LINE__." " . "$a == $f: "; var_dump ($a == $f);
   echo "line:".__LINE__." " . "$a == $g: "; var_dump ($a == $g);
   echo "line:".__LINE__." " . "$a == $h: "; var_dump ($a == $h);
   echo "line:".__LINE__." " . "$a == $i: "; var_dump ($a == $i);

   echo "line:".__LINE__." " . "$a == $k: "; var_dump ($a == $k);

   echo "line:".__LINE__." " . "$a == $m: "; var_dump ($a == $m);
   echo "line:".__LINE__." " . "$b == $a: "; var_dump ($b == $a);
   echo "line:".__LINE__." " . "$b == $b: "; var_dump ($b == $b);
   echo "line:".__LINE__." " . "$b == $c: "; var_dump ($b == $c);
   echo "line:".__LINE__." " . "$b == $d: "; var_dump ($b == $d);
   echo "line:".__LINE__." " . "$b == $e: "; var_dump ($b == $e);
   echo "line:".__LINE__." " . "$b == $f: "; var_dump ($b == $f);
   echo "line:".__LINE__." " . "$b == $g: "; var_dump ($b == $g);
   echo "line:".__LINE__." " . "$b == $h: "; var_dump ($b == $h);
   echo "line:".__LINE__." " . "$b == $i: "; var_dump ($b == $i);

   echo "line:".__LINE__." " . "$b == $k: "; var_dump ($b == $k);

   echo "line:".__LINE__." " . "$b == $m: "; var_dump ($b == $m);
   echo "line:".__LINE__." " . "$c == $a: "; var_dump ($c == $a);
   echo "line:".__LINE__." " . "$c == $b: "; var_dump ($c == $b);
   echo "line:".__LINE__." " . "$c == $c: "; var_dump ($c == $c);
   echo "line:".__LINE__." " . "$c == $d: "; var_dump ($c == $d);
   echo "line:".__LINE__." " . "$c == $e: "; var_dump ($c == $e);
   echo "line:".__LINE__." " . "$c == $f: "; var_dump ($c == $f);
   echo "line:".__LINE__." " . "$c == $g: "; var_dump ($c == $g);
   echo "line:".__LINE__." " . "$c == $h: "; var_dump ($c == $h);
   echo "line:".__LINE__." " . "$c == $i: "; var_dump ($c == $i);

   echo "line:".__LINE__." " . "$c == $k: "; var_dump ($c == $k);

   echo "line:".__LINE__." " . "$c == $m: "; var_dump ($c == $m);
   echo "line:".__LINE__." " . "$d == $a: "; var_dump ($d == $a);
   echo "line:".__LINE__." " . "$d == $b: "; var_dump ($d == $b);
   echo "line:".__LINE__." " . "$d == $c: "; var_dump ($d == $c);
   echo "line:".__LINE__." " . "$d == $d: "; var_dump ($d == $d);
   echo "line:".__LINE__." " . "$d == $e: "; var_dump ($d == $e);
   echo "line:".__LINE__." " . "$d == $f: "; var_dump ($d == $f);
   echo "line:".__LINE__." " . "$d == $g: "; var_dump ($d == $g);
   echo "line:".__LINE__." " . "$d == $h: "; var_dump ($d == $h);
   echo "line:".__LINE__." " . "$d == $i: "; var_dump ($d == $i);

   echo "line:".__LINE__." " . "$d == $k: "; var_dump ($d == $k);

   echo "line:".__LINE__." " . "$d == $m: "; var_dump ($d == $m);
   echo "line:".__LINE__." " . "$e == $a: "; var_dump ($e == $a);
   echo "line:".__LINE__." " . "$e == $b: "; var_dump ($e == $b);
   echo "line:".__LINE__." " . "$e == $c: "; var_dump ($e == $c);
   echo "line:".__LINE__." " . "$e == $d: "; var_dump ($e == $d);
   echo "line:".__LINE__." " . "$e == $e: "; var_dump ($e == $e);
   echo "line:".__LINE__." " . "$e == $f: "; var_dump ($e == $f);
   echo "line:".__LINE__." " . "$e == $g: "; var_dump ($e == $g);
   echo "line:".__LINE__." " . "$e == $h: "; var_dump ($e == $h);
   echo "line:".__LINE__." " . "$e == $i: "; var_dump ($e == $i);

   echo "line:".__LINE__." " . "$e == $k: "; var_dump ($e == $k);

   echo "line:".__LINE__." " . "$e == $m: "; var_dump ($e == $m);
   echo "line:".__LINE__." " . "$f == $a: "; var_dump ($f == $a);
   echo "line:".__LINE__." " . "$f == $b: "; var_dump ($f == $b);
   echo "line:".__LINE__." " . "$f == $c: "; var_dump ($f == $c);
   echo "line:".__LINE__." " . "$f == $d: "; var_dump ($f == $d);
   echo "line:".__LINE__." " . "$f == $e: "; var_dump ($f == $e);
   echo "line:".__LINE__." " . "$f == $f: "; var_dump ($f == $f);
   echo "line:".__LINE__." " . "$f == $g: "; var_dump ($f == $g);
   echo "line:".__LINE__." " . "$f == $h: "; var_dump ($f == $h);
   echo "line:".__LINE__." " . "$f == $i: "; var_dump ($f == $i);

   echo "line:".__LINE__." " . "$f == $k: "; var_dump ($f == $k);

   echo "line:".__LINE__." " . "$f == $m: "; var_dump ($f == $m);
   echo "line:".__LINE__." " . "$g == $a: "; var_dump ($g == $a);
   echo "line:".__LINE__." " . "$g == $b: "; var_dump ($g == $b);
   echo "line:".__LINE__." " . "$g == $c: "; var_dump ($g == $c);
   echo "line:".__LINE__." " . "$g == $d: "; var_dump ($g == $d);
   echo "line:".__LINE__." " . "$g == $e: "; var_dump ($g == $e);
   echo "line:".__LINE__." " . "$g == $f: "; var_dump ($g == $f);
   echo "line:".__LINE__." " . "$g == $g: "; var_dump ($g == $g);
   echo "line:".__LINE__." " . "$g == $h: "; var_dump ($g == $h);
   echo "line:".__LINE__." " . "$g == $i: "; var_dump ($g == $i);
   echo "line:".__LINE__." " . "$g == $j: "; var_dump ($g == $j);
   echo "line:".__LINE__." " . "$g == $k: "; var_dump ($g == $k);
   echo "line:".__LINE__." " . "$g == $l: "; var_dump ($g == $l);
   echo "line:".__LINE__." " . "$g == $m: "; var_dump ($g == $m);
   echo "line:".__LINE__." " . "$h == $a: "; var_dump ($h == $a);
   echo "line:".__LINE__." " . "$h == $b: "; var_dump ($h == $b);
   echo "line:".__LINE__." " . "$h == $c: "; var_dump ($h == $c);
   echo "line:".__LINE__." " . "$h == $d: "; var_dump ($h == $d);
   echo "line:".__LINE__." " . "$h == $e: "; var_dump ($h == $e);
   echo "line:".__LINE__." " . "$h == $f: "; var_dump ($h == $f);
   echo "line:".__LINE__." " . "$h == $g: "; var_dump ($h == $g);
   echo "line:".__LINE__." " . "$h == $h: "; var_dump ($h == $h);
   echo "line:".__LINE__." " . "$h == $i: "; var_dump ($h == $i);
   echo "line:".__LINE__." " . "$h == $j: "; var_dump ($h == $j);
   echo "line:".__LINE__." " . "$h == $k: "; var_dump ($h == $k);
   echo "line:".__LINE__." " . "$h == $l: "; var_dump ($h == $l);
   echo "line:".__LINE__." " . "$h == $m: "; var_dump ($h == $m);
   echo "line:".__LINE__." " . "$i == $a: "; var_dump ($i == $a);
   echo "line:".__LINE__." " . "$i == $b: "; var_dump ($i == $b);
   echo "line:".__LINE__." " . "$i == $c: "; var_dump ($i == $c);
   echo "line:".__LINE__." " . "$i == $d: "; var_dump ($i == $d);
   echo "line:".__LINE__." " . "$i == $e: "; var_dump ($i == $e);
   echo "line:".__LINE__." " . "$i == $f: "; var_dump ($i == $f);
   echo "line:".__LINE__." " . "$i == $g: "; var_dump ($i == $g);
   echo "line:".__LINE__." " . "$i == $h: "; var_dump ($i == $h);
   echo "line:".__LINE__." " . "$i == $i: "; var_dump ($i == $i);
   echo "line:".__LINE__." " . "$i == $j: "; var_dump ($i == $j);
   echo "line:".__LINE__." " . "$i == $k: "; var_dump ($i == $k);
   echo "line:".__LINE__." " . "$i == $l: "; var_dump ($i == $l);
   echo "line:".__LINE__." " . "$i == $m: "; var_dump ($i == $m);






   echo "line:".__LINE__." " . "$j == $g: "; var_dump ($j == $g);
   echo "line:".__LINE__." " . "$j == $h: "; var_dump ($j == $h);
   echo "line:".__LINE__." " . "$j == $i: "; var_dump ($j == $i);
   echo "line:".__LINE__." " . "$j == $j: "; var_dump ($j == $j);

   echo "line:".__LINE__." " . "$j == $l: "; var_dump ($j == $l);

   echo "line:".__LINE__." " . "$k == $a: "; var_dump ($k == $a);
   echo "line:".__LINE__." " . "$k == $b: "; var_dump ($k == $b);
   echo "line:".__LINE__." " . "$k == $c: "; var_dump ($k == $c);
   echo "line:".__LINE__." " . "$k == $d: "; var_dump ($k == $d);
   echo "line:".__LINE__." " . "$k == $e: "; var_dump ($k == $e);
   echo "line:".__LINE__." " . "$k == $f: "; var_dump ($k == $f);
   echo "line:".__LINE__." " . "$k == $g: "; var_dump ($k == $g);
   echo "line:".__LINE__." " . "$k == $h: "; var_dump ($k == $h);
   echo "line:".__LINE__." " . "$k == $i: "; var_dump ($k == $i);

   echo "line:".__LINE__." " . "$k == $k: "; var_dump ($k == $k);

   echo "line:".__LINE__." " . "$k == $m: "; var_dump ($k == $m);






   echo "line:".__LINE__." " . "$l == $g: "; var_dump ($l == $g);
   echo "line:".__LINE__." " . "$l == $h: "; var_dump ($l == $h);
   echo "line:".__LINE__." " . "$l == $i: "; var_dump ($l == $i);
   echo "line:".__LINE__." " . "$l == $j: "; var_dump ($l == $j);

   echo "line:".__LINE__." " . "$l == $l: "; var_dump ($l == $l);

   echo "line:".__LINE__." " . "$m == $a: "; var_dump ($m == $a);
   echo "line:".__LINE__." " . "$m == $b: "; var_dump ($m == $b);
   echo "line:".__LINE__." " . "$m == $c: "; var_dump ($m == $c);
   echo "line:".__LINE__." " . "$m == $d: "; var_dump ($m == $d);
   echo "line:".__LINE__." " . "$m == $e: "; var_dump ($m == $e);
   echo "line:".__LINE__." " . "$m == $f: "; var_dump ($m == $f);
   echo "line:".__LINE__." " . "$m == $g: "; var_dump ($m == $g);
   echo "line:".__LINE__." " . "$m == $h: "; var_dump ($m == $h);
   echo "line:".__LINE__." " . "$m == $i: "; var_dump ($m == $i);

   echo "line:".__LINE__." " . "$m == $k: "; var_dump ($m == $k);

   echo "line:".__LINE__." " . "$m == $m: "; var_dump ($m == $m);

   echo "line:".__LINE__." " . "$n == $a: "; var_dump ($n == $a);
   echo "line:".__LINE__." " . "$n == $b: "; var_dump ($n == $b);
   echo "line:".__LINE__." " . "$n == $c: "; var_dump ($n == $c);
   echo "line:".__LINE__." " . "$n == $d: "; var_dump ($n == $d);
   echo "line:".__LINE__." " . "$n == $e: "; var_dump ($n == $e);
   echo "line:".__LINE__." " . "$n == $f: "; var_dump ($n == $f);
   echo "line:".__LINE__." " . "$n == $g: "; var_dump ($n == $g);
   echo "line:".__LINE__." " . "$n == $h: "; var_dump ($n == $h);
   echo "line:".__LINE__." " . "$n == $i: "; var_dump ($n == $i);

   echo "line:".__LINE__." " . "$n == $k: "; var_dump ($n == $k);

   echo "line:".__LINE__." " . "$n == $m: "; var_dump ($n == $m);
   echo "line:".__LINE__." " . "$n == $n: "; var_dump ($n == $n);
   echo "line:".__LINE__." " . "$a == $n: "; var_dump ($a == $n);
   echo "line:".__LINE__." " . "$b == $n: "; var_dump ($b == $n);
   echo "line:".__LINE__." " . "$c == $n: "; var_dump ($c == $n);
   echo "line:".__LINE__." " . "$d == $n: "; var_dump ($d == $n);
   echo "line:".__LINE__." " . "$e == $n: "; var_dump ($e == $n);
   echo "line:".__LINE__." " . "$f == $n: "; var_dump ($f == $n);
   echo "line:".__LINE__." " . "$g == $n: "; var_dump ($g == $n);
   echo "line:".__LINE__." " . "$h == $n: "; var_dump ($h == $n);
   echo "line:".__LINE__." " . "$i == $n: "; var_dump ($i == $n);

   echo "line:".__LINE__." " . "$k == $n: "; var_dump ($k == $n);

   echo "line:".__LINE__." " . "$m == $n: "; var_dump ($m == $n);
   
                                                         
  foreach ($arr as $o) {
     echo "line:".__LINE__." " . "$o === $a: "; var_dump ($o === $a);
     echo "line:".__LINE__." " . "$o === $b: "; var_dump ($o === $b);

     echo "line:".__LINE__." " . "$o === $d: "; var_dump ($o === $d);
     echo "line:".__LINE__." " . "$o === $e: "; var_dump ($o === $e);
     echo "line:".__LINE__." " . "$o === $f: "; var_dump ($o === $f);
     echo "line:".__LINE__." " . "$o === $g: "; var_dump ($o === $g);
     echo "line:".__LINE__." " . "$o === $h: "; var_dump ($o === $h);
     echo "line:".__LINE__." " . "$o === $i: "; var_dump ($o === $i);
     echo "line:".__LINE__." " . "$o === $j: "; var_dump ($o === $j);
     echo "line:".__LINE__." " . "$o === $k: "; var_dump ($o === $k);
     echo "line:".__LINE__." " . "$o === $l: "; var_dump ($o === $l);
     echo "line:".__LINE__." " . "$o === $m: "; var_dump ($o === $m);
     echo "line:".__LINE__." " . "$o === $n: "; var_dump ($o === $n);
     echo "line:".__LINE__." " . "$a === $o: "; var_dump ($a === $o);
     echo "line:".__LINE__." " . "$b === $o: "; var_dump ($b === $o);

     echo "line:".__LINE__." " . "$d === $o: "; var_dump ($d === $o);
     echo "line:".__LINE__." " . "$e === $o: "; var_dump ($e === $o);
     echo "line:".__LINE__." " . "$f === $o: "; var_dump ($f === $o);
     echo "line:".__LINE__." " . "$g === $o: "; var_dump ($g === $o);
     echo "line:".__LINE__." " . "$h === $o: "; var_dump ($h === $o);
     echo "line:".__LINE__." " . "$i === $o: "; var_dump ($i === $o);
     echo "line:".__LINE__." " . "$j === $o: "; var_dump ($j === $o);
     echo "line:".__LINE__." " . "$k === $o: "; var_dump ($k === $o);
     echo "line:".__LINE__." " . "$l === $o: "; var_dump ($l === $o);
     echo "line:".__LINE__." " . "$m === $o: "; var_dump ($m === $o);
     echo "line:".__LINE__." " . "$n === $o: "; var_dump ($n === $o);
  }

   echo "line:".__LINE__." " . "$a === $a: "; var_dump ($a === $a);


   echo "line:".__LINE__." " . "$a === $d: "; var_dump ($a === $d);
   echo "line:".__LINE__." " . "$a === $e: "; var_dump ($a === $e);
   echo "line:".__LINE__." " . "$a === $f: "; var_dump ($a === $f);
   echo "line:".__LINE__." " . "$a === $g: "; var_dump ($a === $g);
   echo "line:".__LINE__." " . "$a === $h: "; var_dump ($a === $h);
   echo "line:".__LINE__." " . "$a === $i: "; var_dump ($a === $i);
   echo "line:".__LINE__." " . "$a === $j: "; var_dump ($a === $j);
   echo "line:".__LINE__." " . "$a === $k: "; var_dump ($a === $k);
   echo "line:".__LINE__." " . "$a === $l: "; var_dump ($a === $l);
   echo "line:".__LINE__." " . "$a === $m: "; var_dump ($a === $m);
   echo "line:".__LINE__." " . "$b === $a: "; var_dump ($b === $a);
   echo "line:".__LINE__." " . "$b === $b: "; var_dump ($b === $b);
   echo "line:".__LINE__." " . "$b === $c: "; var_dump ($b === $c);
   echo "line:".__LINE__." " . "$b === $d: "; var_dump ($b === $d);
   echo "line:".__LINE__." " . "$b === $e: "; var_dump ($b === $e);
   echo "line:".__LINE__." " . "$b === $f: "; var_dump ($b === $f);
   echo "line:".__LINE__." " . "$b === $g: "; var_dump ($b === $g);
   echo "line:".__LINE__." " . "$b === $h: "; var_dump ($b === $h);
   echo "line:".__LINE__." " . "$b === $i: "; var_dump ($b === $i);
   echo "line:".__LINE__." " . "$b === $j: "; var_dump ($b === $j);
   echo "line:".__LINE__." " . "$b === $k: "; var_dump ($b === $k);
   echo "line:".__LINE__." " . "$b === $l: "; var_dump ($b === $l);
   echo "line:".__LINE__." " . "$b === $m: "; var_dump ($b === $m);
   echo "line:".__LINE__." " . "$c === $a: "; var_dump ($c === $a);
   echo "line:".__LINE__." " . "$c === $b: "; var_dump ($c === $b);
   echo "line:".__LINE__." " . "$c === $c: "; var_dump ($c === $c);
   echo "line:".__LINE__." " . "$c === $d: "; var_dump ($c === $d);
   echo "line:".__LINE__." " . "$c === $e: "; var_dump ($c === $e);
   echo "line:".__LINE__." " . "$c === $f: "; var_dump ($c === $f);
   echo "line:".__LINE__." " . "$c === $g: "; var_dump ($c === $g);
   echo "line:".__LINE__." " . "$c === $h: "; var_dump ($c === $h);
   echo "line:".__LINE__." " . "$c === $i: "; var_dump ($c === $i);
   echo "line:".__LINE__." " . "$c === $j: "; var_dump ($c === $j);
   echo "line:".__LINE__." " . "$c === $k: "; var_dump ($c === $k);
   echo "line:".__LINE__." " . "$c === $l: "; var_dump ($c === $l);
   echo "line:".__LINE__." " . "$c === $m: "; var_dump ($c === $m);
   echo "line:".__LINE__." " . "$d === $a: "; var_dump ($d === $a);
   echo "line:".__LINE__." " . "$d === $b: "; var_dump ($d === $b);
   echo "line:".__LINE__." " . "$d === $c: "; var_dump ($d === $c);
   echo "line:".__LINE__." " . "$d === $d: "; var_dump ($d === $d);
   echo "line:".__LINE__." " . "$d === $e: "; var_dump ($d === $e);
   echo "line:".__LINE__." " . "$d === $f: "; var_dump ($d === $f);
   echo "line:".__LINE__." " . "$d === $g: "; var_dump ($d === $g);
   echo "line:".__LINE__." " . "$d === $h: "; var_dump ($d === $h);
   echo "line:".__LINE__." " . "$d === $i: "; var_dump ($d === $i);
   echo "line:".__LINE__." " . "$d === $j: "; var_dump ($d === $j);
   echo "line:".__LINE__." " . "$d === $k: "; var_dump ($d === $k);
   echo "line:".__LINE__." " . "$d === $l: "; var_dump ($d === $l);
   echo "line:".__LINE__." " . "$d === $m: "; var_dump ($d === $m);
   echo "line:".__LINE__." " . "$e === $a: "; var_dump ($e === $a);
   echo "line:".__LINE__." " . "$e === $b: "; var_dump ($e === $b);
   echo "line:".__LINE__." " . "$e === $c: "; var_dump ($e === $c);
   echo "line:".__LINE__." " . "$e === $d: "; var_dump ($e === $d);
   echo "line:".__LINE__." " . "$e === $e: "; var_dump ($e === $e);
   echo "line:".__LINE__." " . "$e === $f: "; var_dump ($e === $f);
   echo "line:".__LINE__." " . "$e === $g: "; var_dump ($e === $g);
   echo "line:".__LINE__." " . "$e === $h: "; var_dump ($e === $h);
   echo "line:".__LINE__." " . "$e === $i: "; var_dump ($e === $i);
   echo "line:".__LINE__." " . "$e === $j: "; var_dump ($e === $j);
   echo "line:".__LINE__." " . "$e === $k: "; var_dump ($e === $k);
   echo "line:".__LINE__." " . "$e === $l: "; var_dump ($e === $l);
   echo "line:".__LINE__." " . "$e === $m: "; var_dump ($e === $m);
   echo "line:".__LINE__." " . "$f === $a: "; var_dump ($f === $a);
   echo "line:".__LINE__." " . "$f === $b: "; var_dump ($f === $b);
   echo "line:".__LINE__." " . "$f === $c: "; var_dump ($f === $c);
   echo "line:".__LINE__." " . "$f === $d: "; var_dump ($f === $d);
   echo "line:".__LINE__." " . "$f === $e: "; var_dump ($f === $e);
   echo "line:".__LINE__." " . "$f === $f: "; var_dump ($f === $f);
   echo "line:".__LINE__." " . "$f === $g: "; var_dump ($f === $g);
   echo "line:".__LINE__." " . "$f === $h: "; var_dump ($f === $h);
   echo "line:".__LINE__." " . "$f === $i: "; var_dump ($f === $i);
   echo "line:".__LINE__." " . "$f === $j: "; var_dump ($f === $j);
   echo "line:".__LINE__." " . "$f === $k: "; var_dump ($f === $k);
   echo "line:".__LINE__." " . "$f === $l: "; var_dump ($f === $l);
   echo "line:".__LINE__." " . "$f === $m: "; var_dump ($f === $m);
   echo "line:".__LINE__." " . "$g === $a: "; var_dump ($g === $a);
   echo "line:".__LINE__." " . "$g === $b: "; var_dump ($g === $b);
   echo "line:".__LINE__." " . "$g === $c: "; var_dump ($g === $c);
   echo "line:".__LINE__." " . "$g === $d: "; var_dump ($g === $d);
   echo "line:".__LINE__." " . "$g === $e: "; var_dump ($g === $e);
   echo "line:".__LINE__." " . "$g === $f: "; var_dump ($g === $f);
   echo "line:".__LINE__." " . "$g === $g: "; var_dump ($g === $g);
   echo "line:".__LINE__." " . "$g === $h: "; var_dump ($g === $h);
   echo "line:".__LINE__." " . "$g === $i: "; var_dump ($g === $i);
   echo "line:".__LINE__." " . "$g === $j: "; var_dump ($g === $j);
   echo "line:".__LINE__." " . "$g === $k: "; var_dump ($g === $k);
   echo "line:".__LINE__." " . "$g === $l: "; var_dump ($g === $l);
   echo "line:".__LINE__." " . "$g === $m: "; var_dump ($g === $m);
   echo "line:".__LINE__." " . "$h === $a: "; var_dump ($h === $a);
   echo "line:".__LINE__." " . "$h === $b: "; var_dump ($h === $b);
   echo "line:".__LINE__." " . "$h === $c: "; var_dump ($h === $c);
   echo "line:".__LINE__." " . "$h === $d: "; var_dump ($h === $d);
   echo "line:".__LINE__." " . "$h === $e: "; var_dump ($h === $e);
   echo "line:".__LINE__." " . "$h === $f: "; var_dump ($h === $f);
   echo "line:".__LINE__." " . "$h === $g: "; var_dump ($h === $g);
   echo "line:".__LINE__." " . "$h === $h: "; var_dump ($h === $h);
   echo "line:".__LINE__." " . "$h === $i: "; var_dump ($h === $i);
   echo "line:".__LINE__." " . "$h === $j: "; var_dump ($h === $j);
   echo "line:".__LINE__." " . "$h === $k: "; var_dump ($h === $k);
   echo "line:".__LINE__." " . "$h === $l: "; var_dump ($h === $l);
   echo "line:".__LINE__." " . "$h === $m: "; var_dump ($h === $m);
   echo "line:".__LINE__." " . "$i === $a: "; var_dump ($i === $a);
   echo "line:".__LINE__." " . "$i === $b: "; var_dump ($i === $b);
   echo "line:".__LINE__." " . "$i === $c: "; var_dump ($i === $c);
   echo "line:".__LINE__." " . "$i === $d: "; var_dump ($i === $d);
   echo "line:".__LINE__." " . "$i === $e: "; var_dump ($i === $e);
   echo "line:".__LINE__." " . "$i === $f: "; var_dump ($i === $f);
   echo "line:".__LINE__." " . "$i === $g: "; var_dump ($i === $g);
   echo "line:".__LINE__." " . "$i === $h: "; var_dump ($i === $h);
   echo "line:".__LINE__." " . "$i === $i: "; var_dump ($i === $i);
   echo "line:".__LINE__." " . "$i === $j: "; var_dump ($i === $j);
   echo "line:".__LINE__." " . "$i === $k: "; var_dump ($i === $k);
   echo "line:".__LINE__." " . "$i === $l: "; var_dump ($i === $l);
   echo "line:".__LINE__." " . "$i === $m: "; var_dump ($i === $m);
   echo "line:".__LINE__." " . "$j === $a: "; var_dump ($j === $a);
   echo "line:".__LINE__." " . "$j === $b: "; var_dump ($j === $b);
   echo "line:".__LINE__." " . "$j === $c: "; var_dump ($j === $c);
   echo "line:".__LINE__." " . "$j === $d: "; var_dump ($j === $d);
   echo "line:".__LINE__." " . "$j === $e: "; var_dump ($j === $e);
   echo "line:".__LINE__." " . "$j === $f: "; var_dump ($j === $f);
   echo "line:".__LINE__." " . "$j === $g: "; var_dump ($j === $g);
   echo "line:".__LINE__." " . "$j === $h: "; var_dump ($j === $h);
   echo "line:".__LINE__." " . "$j === $i: "; var_dump ($j === $i);
   echo "line:".__LINE__." " . "$j === $j: "; var_dump ($j === $j);
   echo "line:".__LINE__." " . "$j === $k: "; var_dump ($j === $k);
   echo "line:".__LINE__." " . "$j === $l: "; var_dump ($j === $l);
   echo "line:".__LINE__." " . "$j === $m: "; var_dump ($j === $m);
   echo "line:".__LINE__." " . "$k === $a: "; var_dump ($k === $a);
   echo "line:".__LINE__." " . "$k === $b: "; var_dump ($k === $b);
   echo "line:".__LINE__." " . "$k === $c: "; var_dump ($k === $c);
   echo "line:".__LINE__." " . "$k === $d: "; var_dump ($k === $d);
   echo "line:".__LINE__." " . "$k === $e: "; var_dump ($k === $e);
   echo "line:".__LINE__." " . "$k === $f: "; var_dump ($k === $f);
   echo "line:".__LINE__." " . "$k === $g: "; var_dump ($k === $g);
   echo "line:".__LINE__." " . "$k === $h: "; var_dump ($k === $h);
   echo "line:".__LINE__." " . "$k === $i: "; var_dump ($k === $i);
   echo "line:".__LINE__." " . "$k === $j: "; var_dump ($k === $j);
   echo "line:".__LINE__." " . "$k === $k: "; var_dump ($k === $k);
   echo "line:".__LINE__." " . "$k === $l: "; var_dump ($k === $l);
   echo "line:".__LINE__." " . "$k === $m: "; var_dump ($k === $m);
   echo "line:".__LINE__." " . "$l === $a: "; var_dump ($l === $a);
   echo "line:".__LINE__." " . "$l === $b: "; var_dump ($l === $b);
   echo "line:".__LINE__." " . "$l === $c: "; var_dump ($l === $c);
   echo "line:".__LINE__." " . "$l === $d: "; var_dump ($l === $d);
   echo "line:".__LINE__." " . "$l === $e: "; var_dump ($l === $e);
   echo "line:".__LINE__." " . "$l === $f: "; var_dump ($l === $f);
   echo "line:".__LINE__." " . "$l === $g: "; var_dump ($l === $g);
   echo "line:".__LINE__." " . "$l === $h: "; var_dump ($l === $h);
   echo "line:".__LINE__." " . "$l === $i: "; var_dump ($l === $i);
   echo "line:".__LINE__." " . "$l === $j: "; var_dump ($l === $j);
   echo "line:".__LINE__." " . "$l === $k: "; var_dump ($l === $k);
   echo "line:".__LINE__." " . "$l === $l: "; var_dump ($l === $l);
   echo "line:".__LINE__." " . "$l === $m: "; var_dump ($l === $m);
   echo "line:".__LINE__." " . "$m === $a: "; var_dump ($m === $a);
   echo "line:".__LINE__." " . "$m === $b: "; var_dump ($m === $b);
   echo "line:".__LINE__." " . "$m === $c: "; var_dump ($m === $c);
   echo "line:".__LINE__." " . "$m === $d: "; var_dump ($m === $d);
   echo "line:".__LINE__." " . "$m === $e: "; var_dump ($m === $e);
   echo "line:".__LINE__." " . "$m === $f: "; var_dump ($m === $f);
   echo "line:".__LINE__." " . "$m === $g: "; var_dump ($m === $g);
   echo "line:".__LINE__." " . "$m === $h: "; var_dump ($m === $h);
   echo "line:".__LINE__." " . "$m === $i: "; var_dump ($m === $i);
   echo "line:".__LINE__." " . "$m === $j: "; var_dump ($m === $j);
   echo "line:".__LINE__." " . "$m === $k: "; var_dump ($m === $k);
   echo "line:".__LINE__." " . "$m === $l: "; var_dump ($m === $l);
   echo "line:".__LINE__." " . "$m === $m: "; var_dump ($m === $m);

   echo "line:".__LINE__." " . "$n === $a: "; var_dump ($n === $a);
   echo "line:".__LINE__." " . "$n === $b: "; var_dump ($n === $b);
   echo "line:".__LINE__." " . "$n === $c: "; var_dump ($n === $c);
   echo "line:".__LINE__." " . "$n === $d: "; var_dump ($n === $d);
   echo "line:".__LINE__." " . "$n === $e: "; var_dump ($n === $e);
   echo "line:".__LINE__." " . "$n === $f: "; var_dump ($n === $f);
   echo "line:".__LINE__." " . "$n === $g: "; var_dump ($n === $g);
   echo "line:".__LINE__." " . "$n === $h: "; var_dump ($n === $h);
   echo "line:".__LINE__." " . "$n === $i: "; var_dump ($n === $i);
   echo "line:".__LINE__." " . "$n === $j: "; var_dump ($n === $j);
   echo "line:".__LINE__." " . "$n === $k: "; var_dump ($n === $k);
   echo "line:".__LINE__." " . "$n === $l: "; var_dump ($n === $l);
   echo "line:".__LINE__." " . "$n === $m: "; var_dump ($n === $m);
   echo "line:".__LINE__." " . "$n === $n: "; var_dump ($n === $n);
   echo "line:".__LINE__." " . "$a === $n: "; var_dump ($a === $n);
   echo "line:".__LINE__." " . "$b === $n: "; var_dump ($b === $n);
   echo "line:".__LINE__." " . "$c === $n: "; var_dump ($c === $n);
   echo "line:".__LINE__." " . "$d === $n: "; var_dump ($d === $n);
   echo "line:".__LINE__." " . "$e === $n: "; var_dump ($e === $n);
   echo "line:".__LINE__." " . "$f === $n: "; var_dump ($f === $n);
   echo "line:".__LINE__." " . "$g === $n: "; var_dump ($g === $n);
   echo "line:".__LINE__." " . "$h === $n: "; var_dump ($h === $n);
   echo "line:".__LINE__." " . "$i === $n: "; var_dump ($i === $n);
   echo "line:".__LINE__." " . "$j === $n: "; var_dump ($j === $n);
   echo "line:".__LINE__." " . "$k === $n: "; var_dump ($k === $n);
   echo "line:".__LINE__." " . "$l === $n: "; var_dump ($l === $n);
   echo "line:".__LINE__." " . "$m === $n: "; var_dump ($m === $n);

/*
  for ($i = 'a'; $i <= 'm'; $i++) {
    for ($j = 'a'; $j <= 'm'; $j++) {
       echo "line:".__LINE__." " . "   echo "line:".__LINE__." " . \"$$i == $$j: \"; var_dump ($$i == $$j);\n";
    }
  }
*/


  $arr2 = array (0, 1, -1, '0', '1', '-1', '', null, true, false, '10', -100, 1.5);
  foreach ($arr2 as $x)
    foreach ($arr2 as $y) {
      echo "line:".__LINE__." " . gettype ($x) . " " . gettype ($y) . " $x < $y: "; var_dump ($x < $y);
      echo "line:".__LINE__." " . gettype ($x) . " " . gettype ($y) . " $x > $y: "; var_dump ($x > $y);
      echo "line:".__LINE__." " . gettype ($x) . " " . gettype ($y) . " $x <= $y: "; var_dump ($x <= $y);
      echo "line:".__LINE__." " . gettype ($x) . " " . gettype ($y) . " $x >= $y: "; var_dump ($x >= $y);
    }

  $arr3 = array ("1e1", "     10e-0", "10e+0", "   1e1", "1e10", "10000000000", "123456789012345678901234567890", "123456789012345678901234567891", "0", "-0", 10, ".1e2", "", false, 012);
  foreach ($arr3 as $x)
    foreach ($arr3 as $y) {
      echo "line:".__LINE__." " . gettype ($x) . " " . gettype ($y) . " $x < $y: "; var_dump ($x < $y);
      echo "line:".__LINE__." " . gettype ($x) . " " . gettype ($y) . " $x > $y: "; var_dump ($x > $y);
      echo "line:".__LINE__." " . gettype ($x) . " " . gettype ($y) . " $x <= $y: "; var_dump ($x <= $y);
      echo "line:".__LINE__." " . gettype ($x) . " " . gettype ($y) . " $x >= $y: "; var_dump ($x >= $y);
    }

