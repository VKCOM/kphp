@ok benchmark k2_skip
<?php
  header('WWW-Authenticate: Negotiate');
  header('WWW-Authenticate: NTLM', false);
  header("Location: http://www.example.com/");


  $a_filter = array (1, 2, "asd", null, "1 apple", " ", "-0.0", ".1", ".", false, true, 7.123123, array(1, 2), null);
  var_dump (array_filter ($a_filter, 'is_scalar'));
  var_dump (array_filter ($a_filter, 'is_numeric'));
  var_dump (array_filter ($a_filter, 'is_null'));
  var_dump (array_filter ($a_filter, 'is_bool'));
  var_dump (array_filter ($a_filter, 'is_int'));
  var_dump (array_filter ($a_filter, 'is_integer'));
  var_dump (array_filter ($a_filter, 'is_long'));
  var_dump (array_filter ($a_filter, 'is_float'));
  var_dump (array_filter ($a_filter, 'is_double'));
  var_dump (array_filter ($a_filter, 'is_real'));
  var_dump (array_filter ($a_filter, 'is_string'));
  var_dump (array_filter ($a_filter, 'is_array'));
  var_dump (array_filter ($a_filter, 'is_object'));
  var_dump (array_filter ($a_filter, 'is_numeric'));
  var_dump (array_filter ($a_filter, function($x) { return $x < 4; }));

//  var_dump (explode ('', ',,,,,,,'));
  var_dump (explode (',', ',,,,,,,'));
  var_dump (explode (',,', ',,,,,,,'));
  var_dump (explode (',,,', ',,,,,,,'));
  var_dump (explode (',,,,', ',,,,,,,'));
  var_dump (explode (',,,,,', ',,,,,,,'));
  var_dump (explode (',,,,,,', ',,,,,,,'));
  var_dump (explode (',,,,,,,', ',,,,,,,'));
  var_dump (explode (',,,,,,,,', ',,,,,,,'));

  var_dump (range (5, 10, 2));
  var_dump (range ('0', '9'));
  var_dump (range ('A', 'P'));
  var_dump (range (10, 7));
  var_dump (range ("10", "7", "-1"));

  function test_assign () {
    $x = array ();
    $x[1] = $x;
    if (0) {
      $x = 5;
    }
    $a = array (array ($x));
    $b = array ($x);

    if (1) {
      $b = $a;
    }

    var_dump ($b);
  }
  
  function test_assign2 () {
    $a = array (array (array()));
    $b = $a[0] = $a;
    var_dump ($a);
    var_dump ($b);
  }

  function test_assign3 () {
    $a = array (array (array()));
    $b = $a[0][0] = $a;
    var_dump ($a);
    var_dump ($b);
  }

  function test_assign4 () {
    $a = array (array (array()));
    $a[0][0] = $a;
    var_dump ($a);
  }

  function test_assign5 () {
    $a = array (array (array()));
    $a[0] = $a;
    var_dump ($a);
  }

  test_assign();
  test_assign2();
  test_assign3();
  test_assign4();
  test_assign5();

  $strs = array (" ", "\n", "", " \n", "\r ", "    aasddsd", "asdasdasd     ", "\n\n\n\n\r  asdas  d s   d  \0  fasd    \0\0\0\0\0", "asdasdasdasdasdasdasdas", "A");
  foreach ($strs as $s) {
    var_dump ($s);
    var_dump (ltrim ($s));
    var_dump (rtrim ($s));
    var_dump (trim ($s));
  }


  var_dump (addcslashes ("1\0","\0")."\n");
  var_dump (addcslashes ("", ""));
  var_dump (addcslashes ("", "burp"));
  var_dump (addcslashes ("kaboemkara!", ""));
  var_dump (addcslashes ("foobarbaz", 'bar'));
  var_dump (addcslashes ('foo[ ]', 'A..z'));
  var_dump (addcslashes ('abcdefghijklmnopqrstuvwxyz', "a\145..\160z"));
  var_dump (addcslashes (123, 123));
  var_dump (addcslashes (123, NULL));
  var_dump (addcslashes (NULL, 123));
  var_dump (addcslashes (0, 0));
  var_dump (addcslashes ("\0" , 0));
  var_dump (addcslashes (NULL, NULL));
  var_dump (addcslashes (-1.234578, 3));
  var_dump (addcslashes (" ", " "));
  var_dump (addcslashes ("string\x00with\x00NULL", "\0"));
  var_dump (addslashes ("How's everybody"));
  var_dump (addslashes ('Are you "JOHN"?'));
  var_dump (addslashes ('c:\php\addslashes'));
  var_dump (addslashes ("hello\0world"));
  var_dump (stripslashes (addslashes ("How's everybody")));
  var_dump (stripslashes (addslashes ('Are you "JOHN"?')));
  var_dump (stripslashes (addslashes ('c:\php\addslashes')));
  var_dump (stripslashes (addslashes ("hello\0world")));


  var_dump ((false + true) === false);
  var_dump ($xxxx_bool = (false + true));
  var_dump ($xxxx_bool2 = (false + 1));

//  $x_false = false;
//  var_dump ($x_false--);
//  var_dump (--$x_false);
//  var_dump (++$x_false);
//  var_dump ($x_false++);
  
  $x_true = true;
  @var_dump ($x_true--);
  @var_dump (--$x_true);
  @var_dump (++$x_true);
  @var_dump ($x_true++);
  
  $x = "5.5";
  if (0) {
    $x = 0;
  }
  var_dump ($x++);
  var_dump ($x--);
  var_dump (++$x);
  var_dump (--$x);
  var_dump (++$x);
  var_dump (--$x);
  $x = "5.5";
  var_dump (++$x);
  var_dump (--$x);
  var_dump ($x++);
  var_dump ($x++);
  var_dump ($x--);
  var_dump ($x--);

  $to_int = array ("0" => null, "-0" => null, "+0" => null, "-1" => null, "+1" => null, " 1" => null, "1 " => null, " 0" => null, "0 " => null, "-" => null, "+" => null, "01" => null, "1234567890" => null, "123456789" => null, "2147383648" => null, "-2147483648" => null, "-2147483647" => null, "2147483647" => null, "-1000000000" => null, "0000000000000007" => null, "a" => null, "" => null, "abacaba" => null, " -2147483648" => null);
  var_dump ($to_int);

  $str_test_1 = "5";
  $str_test_2 += $str_test_1;
  $str_test_3 = $str_test_3 + $str_test_1;
  var_dump ($str_test_2);
  var_dump ($str_test_3);

  var_dump (trim(', ', ', '));
  var_dump (trim(' ,', ', '));
  var_dump (trim(' , , test , , ,', ', '));

#ifndef KPHP
  if (0) {
#endif
    warning ("test");
#ifndef KPHP
  }
#endif

  $strs = array ("a", "ab", "aa", "ac", "aaa", "aaa ", "", "a\x0", "ab\x0", "aa\x0", "ac\x0", "aaa\x0", "aaa\x0 ", "\x0");
  foreach ($strs as $s1)
    foreach ($strs as $s2) {
      var_dump ($s1 < $s2);
      var_dump ($s1 == $s2);
      var_dump ($s1 > $s2);
    }

  $arr1 = false;
  $arr1[] = 1;
  var_dump ($arr1);

  $arr1 = "";
//  $arr1[] = 2;
  var_dump ($arr1);

  $arr1 = null;
  $arr1[] = 3;
  var_dump ($arr1);

/*
  $r = array (1, 5);
  foreach (array_slice($r, 1, $r[0]) as &$item) {
    var_dump ($item);
  }
*/

  $foo = 'hello world!';
  $foo = ucfirst($foo);
  var_dump ($foo);

  $bar = 'HELLO WORLD!';
  $bar = ucfirst($bar);
  var_dump ($bar);
  $bar = ucfirst(strtolower($bar));
  var_dump ($bar);
  $bar = ucfirst('');
  var_dump ($bar);


  $cst = array();
  $cst['a'] = $cst['b'] = $cst['c'] = array();
  var_dump ($cst);


  $test[0] = '1';
  $test[4] = 2;
  $test[1] = 2;
  $test[2] = 2;
  $test[0] = $test[3] = $test[4];
  var_dump ($test);
  $test = array();


//  var_dump (1e);
  var_dump (1e-3);


  var_dump ('������� ��������');
  var_dump (iconv('WINDOWS-1251', 'UTF-8', '������� ��������'));
  var_dump (rawurlencode(iconv('WINDOWS-1251', 'UTF-8', '������� ��������')));

//  $ip = gethostbynamel ('vk.com');
//  sort ($ip);
//  var_dump ($ip);

//  $ip = gethostbynamel ('www.example.com');
  $ip = gethostbynamel ('localhost');
  var_dump (count ($ip));
  $ip[0] = "127.0.0.1";
  $long = ip2long ($ip[0]);

  var_dump ($ip);
  echo $ip[0]."\n";
  echo long2ip ($long)."\n";
  printf ("%u\n", ip2long ($ip[0]));

  var_dump (inet_pton('127.0.0.1'));
  var_dump (inet_pton('::1'));

  /**
   * @param int $x
   * @return int
   */
  function sign ($x) {
    return $x < 0 ? -1 : ($x > 0 ? 1 : 0);
  }

  $strs = array ("", "A", "a", "ab", "aba", "abA", "BaBA", "bAba", "baBAC", "babaa");
  foreach ($strs as $x)
    foreach ($strs as $y) {
      var_dump (sign (strncmp ($x, $y, strlen ($x) + 1)));
      var_dump (sign (strncmp ($x, $y, strlen ($x))));
      var_dump (sign (strcmp ($x, $y)));
      var_dump (sign (strcasecmp ($x, $y)));
    }

  $ints = array (0, -1, 1, 10, 1000, 1048576, 1000000000, -1000000000);
  foreach ($ints as $x) {
    var_dump (decbin ($x & 0xFFFFFFFF));
    var_dump (dechex ($x & 0xFFFFFFFF));
  }


  var_dump (fmod (4.0, -1.5));
  var_dump (atan2 (0.0, 0.0));
  var_dump (sqrt (4.0));
  var_dump (log (1.0));
  var_dump (log (1.0, 2));
  var_dump (round (log(2.0), 7));
  var_dump (log (2.0, 2));
  var_dump (log (exp (3.5)));


  $aqwe = array ();
  var_dump (array_flip ($aqwe));
  var_dump (array_fill_keys ($aqwe, "asdas"));
  var_dump (array_combine (array (false), array (true)));
  var_dump (strval (array_sum ($aqwe)));
  var_dump (range (false, true));

  var_dump (2.0 % 3.0);
  @var_dump (2 % 4e9);

  /**
   * @param mixed $key
   * @param mixed $fields
   * @return mixed
   */
  function apiWrapObject($key, $fields = array()) {
    return @(array('_' => $key) + $fields);
  }

  $x = 2;
  if (1)
    $x = array();

  var_dump (apiWrapObject ("abacaba"));
  var_dump (apiWrapObject (1, $x));
  var_dump (apiWrapObject (1));
  
  $a = array ('a' => 1);
  $b = array ('b' => 3.14);
  var_dump ($a + $b);

  var_dump (abs(2.5));
  var_dump (abs(-2.5));
  var_dump (abs(2));
  var_dump (abs(-2));

  $aa = array (2);
  array_unshift ($aa, 1.5);
  var_dump ($aa);

  $corr = unserialize ('a:11:{i:0;i:68069;i:1;i:167497;i:2;i:403600;i:3;i:786752;i:4;i:467473;i:5;i:623827;i:6;i:388810;i:7;i:424303;i:8;i:451969;i:9;i:705025;i:10;s:6:"698078";}');
  $id = false;
  $id = 295564;
  var_dump ($corr);
  var_dump (array_unshift ($corr, $id));
  var_dump ($corr);
  var_dump (array_shift ($corr));
  var_dump ($corr);
  
  $x = null;

  $A = array (true, $x, false, 0, 1.5, -3.5, "-0.5", "1", "2.0", "abacaba", array());

  var_dump (array_fill_keys ($A, $A));

  var_dump (array_combine ($A, $A));

  $a["0"] = "3";
  var_dump ($a[0]);
  var_dump ($a["1"]);

  var_dump ($a);

  /**
   * @kphp-required
   * @param int $x
   * @return bool
   */
  function odd ($x) {
    return (bool)($x & 1);
  }

  /**
   * @kphp-required
   * @param int $x
   * @return bool
   */
  function even ($x) {
    return !($x & 1);
  }

  function cube ($n) {
    return $n * $n * $n;
  }


  $xx[1.5] = 2.5;
  var_dump ($xx);

  var_dump (floatval (true));
  var_dump (floatval (2));
  var_dump (floatval (3.0));
  var_dump (floatval ("2.39"));
  var_dump (floatval ("2.3e1sadasdasd"));
  var_dump (floatval ("sadasdasd"));
  var_dump (floatval ("   +1.23"));

#  var_dump (microtime ());
#  var_dump (microtime ());
#  var_dump (microtime ());

#  var_dump (microtime (true));
#  var_dump (microtime (true));
#  var_dump (microtime (true));

  var_dump (array_fill (5, 6, "banana"));
  var_dump (array_fill (-2, 4, "pear"));

  $array11["a"] = 1;
  $array11["b"] = 2;
  $array11["c"] = 3;
  $array11["d"] = 4;
  $array11["e"] = 5;

#  var_dump (array_rand ($array11, 2));
#  var_dump (array_rand ($array11, 2));
#  var_dump (array_rand ($array11, 2));
#  var_dump (array_rand ($array11, 2));
#  var_dump (array_rand ($array11, 2));
  var_dump (array_chunk ($array11, 2));
  var_dump (array_chunk ($array11, 2, true));

  var_dump (array_fill_keys (array_fill (-2, 4, "pear"), "abac"));
  var_dump (array_fill_keys (array_flip ($array11), (string)"prqrq"));

  var_dump (array_map ('odd', $array11));
  var_dump (array_map ('strval', $array11));
  print_r (array_search (4, $array11));

  var_dump (array_combine ($array11, array_map ('odd', $array11)));

  $array12[] = 6;
  $array12[] = 7;
  $array12[] = 8;
  $array12[] = 9;
  $array12[] = 10;
  $array12[] = 11;
  $array12[] = 12;
  $array12[] = 14;

  echo ("Odd :\n");
  var_dump (array_filter ($array11, 'odd'));
  echo ("Even:\n");
  var_dump (array_filter ($array12, 'even'));

  for ($i = 0; $i < 100000; $i++) {
    $aa = array();
    $tmpaa = array();
    $aa["a"] = "apple";
    $aa["2"] = "abacaba";
    $aa["b"] = "banana";
    $tmpaa[] = "x";
    $tmpaa[] = "y";
    $tmpaa[] = "z";
    $aa[4] = $tmpaa;

    array_splice ($aa, -3, -1, $tmpaa);
  }

  $xxxxx = unserialize ('a:4:{i:0;s:1:"#";i:1;s:1:"[";i:2;s:3:"int";i:3;s:1:"]";}');
  var_dump (array_splice ($xxxxx, 0, 4));
  var_dump ($xxxxx);

#  var_dump (microtime (true));
  var_dump ("array_slice");
  var_dump (array_slice ($aa, 1, 100));
  var_dump (array_slice ($aa, 1, 0));
  var_dump (array_slice ($aa, 1, null));
  var_dump (array_slice ($aa, 100, 100));
  var_dump (array_slice ($aa, 100, 0));
  var_dump (array_slice ($aa, 100, null));
  var_dump (array_slice ($aa, -1, 100));
  var_dump (array_slice ($aa, -1, 0));
  var_dump (array_slice ($aa, -1, null));
  var_dump (array_slice ($aa, -100, 100));
  var_dump (array_slice ($aa, -100, 0));
  var_dump (array_slice ($aa, -100, null));

  var_dump (array_slice ($aa, 1, -100));
  var_dump (array_slice ($aa, 1, -1));
  var_dump (array_slice ($aa, 1, -2));
  var_dump (array_slice ($aa, 2, -1));
  var_dump (array_slice ($aa, 100, -100));
  var_dump (array_slice ($aa, 100, -1));
  var_dump (array_slice ($aa, -1, -100));
  var_dump (array_slice ($aa, -1, -1));
  var_dump (array_slice ($aa, -1, -2));
  var_dump (array_slice ($aa, -2, -1));
  var_dump (array_slice ($aa, -100, -100));
  var_dump (array_slice ($aa, -100, -1));
  var_dump ($aa);

  $input = array();
  array_push ($input, 4);
  array_push ($input, "4");
  array_push ($input, "3");
  array_push ($input, 4);
  array_push ($input, 3);
  array_push ($input, "3");

  var_dump (floatval (array_sum ($input)));
  print_r (array_flip ($input));

  var_dump (array_unique ($input));
  var_dump (array_reverse ($input));
  var_dump (array_reverse ($input, true));

  $aba = array();
  array_push ($aba, 1);
  array_push ($aba, 5);

  var_dump (strval (array_sum ($aba)));
  print_r (array_flip ($aba));

  var_dump (array_pop ($aba));
  var_dump (array_pop ($aba));
#  var_dump (array_pop ($aba));
#  var_dump (array_pop ($aba));
#  var_dump (array_pop ($aba));

  $stack = array();
  $stack[2] = 5;
  array_push ($stack, "orange");
  array_push ($stack, "banana");
  $stack["abacaba"] = 6;
  $stack[] = "apple";
  $stack[] = "raspberry";

  $banana = array_pop ($stack);
  var_dump ($banana);

  print_r (array_keys ($stack));
  var_dump (array_values ($stack));

  $cnt = array_unshift ($stack, true);
  var_dump ($stack);
  var_dump ($cnt);

  $fruit = array_shift ($stack);
  var_dump ($stack);
  var_dump ($fruit);

  $array1 = array();
  $array2 = array();

  $array1["blue"] = 1;
  $array1["red"] = 2;
  $array1["green"] = 3;
  $array1["purple"] = 4;

  $array2["green"] = 5;
  $array2["blue"] = 6;
  $array2["yellow"] = 7;
  $array2["cyan"] = 8;

  $array1["a"] = "green";
  $array1[] = "red";
  $array1[] = "blue";

  $array2["b"] = "green";
  $array2[] = "yellow";
  $array2[] = "red";

  var_dump (array_intersect ($array1, $array2));
  var_dump (array_intersect_key ($array1, $array2));
  var_dump (array_diff ($array1, $array2));
  var_dump (array_diff_key ($array1, $array2));

  $tmp = array();
  $tmp[] = "x";
  $tmp[] = "y";
  $tmp[] = "z";

  $b = array();
  $b["a"] = "apple";
  $b["b"] = "banana";
  $b["c"] = $tmp;
  var_dump ($b);

  $a = array();
  $a[] = 2;
  $a[] = "3asdas";
  $a[] = true;
  $a[-5] = 5;
  $a[] = "cadsa";
  $a[] = 2.5;
  $a[] = 72323232;

  var_dump ($a);

  $res = "";
  for ($i = 0; $i < 100000; $i++) {
    $res = implode (";", $a);
  }
#  var_dump (microtime (true));
  echo $res."\n";
  var_dump (array_slice ($a, 1, 2));
  var_dump (array_slice ($a, 1, 2, true));
  var_dump (array_slice ($a, -3, -2, true));
  var_dump (array_slice ($a, -3, -2));
  var_dump (array_slice ($a, -4, 2, true));
  var_dump (array_slice ($a, 4, -1, true));
  var_dump (array_slice ($a, 100, 100, true));
  var_dump (array_slice ($a, 7, 100, true));
  var_dump (array_slice ($a, 0, 100, true));
  var_dump (array_slice ($a, 100, -100, true));
  var_dump (array_slice ($a, 7, -100, true));
  var_dump (array_slice ($a, 0, -100, true));

  var_dump (array_merge ($a, $b));

  $s = "ASDASDASD";
  $x = 12345;
  $y = "abacaba";
  unset ($z);

  for ($i = 0; $i < 100000; $i++) {
    $res = "asadasdsad".$s. 12345678 .$x.$y.$z;
  }
#  var_dump (microtime (true));
  echo $res;

  for ($i = 0; $i < 100000; $i++) {
    $res = explode (",", "1324, 2234, 32343, 4234324, 52434, 42423432, 432, 4, 324, , 324324324234, 324234, ");
  }
#  var_dump (microtime (true));

  $s = implode ("\n", $res);

  echo $s.'\n';
  echo trim ("   ansnansn a      asnnas    ")."|\n";

  $arr = array();

  $arr[0] = 0;
  $arr[1] = 1;
  $arr[2] = 2;
  $arr[3] = 2;
  $arr[4] = 2;
  $arr[5] = 2;
  $arr[6] = 2;
  $arr[7] = 2;
  $arr[8] = 2;
  $arr[9] = 2;
  $arr[10] = 2;
  $arr[11] = 2;
  unset($arr["1"]);
  unset($arr["3"]);
  unset($arr["5"]);
  unset($arr["7"]);
  unset($arr["9"]);
  unset($arr["11"]);

  var_dump ($a);

  $array = array(1, "hello", 1, "world", "hello");
  var_dump(array_count_values($array));

  $a = array(1, 6, 2, -20, 15, 1200, -2500);
  $b = array(0, 7, 2, -20, 11, 1100, -2500);
  $c = array(0, 6, 2, -20, 19, 1000, -2500);
  $d = array(3, 8,-2, -20, 14,  900, -2600);
  $a_f = array_flip($a);
  $b_f = array_flip($b);
  $c_f = array_flip($c);
  $d_f = array_flip($d);
  $i = 1;

  foreach ($a_f as $k=> &$a_f_el) { $a_f_el =$k*2;}
  foreach ($b_f as $k=> &$b_f_el) { $b_f_el =$k*2;}
  foreach ($c_f as $k=> &$c_f_el) { $c_f_el =$k*2;}
  foreach ($d_f as $k=> &$d_f_el) { $d_f_el =$k*2;}

  $c = array_diff_key($a_f, $b_f);
  var_dump($c);

  echo "------ Test $i --------\n";$i++;// 1
  //var_dump(array_diff_key($a_f, $b_f));// keys -> 1, 6, 15, 1200

  //echo "------ Test $i --------\n";$i++;// 2
  //var_dump(array_diff_key($a_f, $c_f));// keys -> 1, 15, 1200

  //echo "------ Test $i --------\n";$i++;// 3
  //var_dump(array_diff_key($a_f, $d_f));// 1, 6, 2, 15, 1200, -2500
  //echo "------ Test $i --------\n";$i++;// 4
  //var_dump(array_diff_key(array_diff_key($a_f, $b_f), $c_f));// 1, 15, 1200


  //echo "------ Test $i --------\n";$i++;// 7
  //var_dump(array_diff_key($b_f, $c_f));// 7, 11, 1100
  //echo "------ Test $i --------\n";$i++;// 8
  //var_dump(array_diff_key($b_f, $d_f));//0, 7, 2, 11, 1100, -2500

	$array1 = array('green', 'red', 'yellow');
	$array2 = array('1', '2', '3');
	$array3 = array(0, 1, 2);
	$array4 = array(true, false, null);
	$a = array_combine($array1, $array1);
	$b = array_combine($array1, $array2);
	$c = array_combine($array1, $array3);
	$d = array_combine($array1, $array4);
	$e = array_combine($array2, $array1);
	$f = array_combine($array2, $array2);
	$g = array_combine($array2, $array3);
	$h = array_combine($array2, $array4);
	$i = array_combine($array3, $array1);
	$j = array_combine($array3, $array2);
	$k = array_combine($array3, $array3);
	$l = array_combine($array3, $array4);
	$m = array_combine($array4, $array1);
	$n = array_combine($array4, $array2);
	$o = array_combine($array4, $array3);
	$p = array_combine($array4, $array4);

  print_r($a);
  print_r($b);
  print_r($c);
  print_r($d);
  print_r($e);
  print_r($f);
  print_r($g);
  print_r($h);
  print_r($i);
  print_r($j);
  print_r($k);
  print_r($l);
  print_r($m);
  print_r($n);
  print_r($o);
  print_r($p);

  $a = array(1, 2, array("a", "b", "c"), 1.0, true);
  var_dump($a);
  print_r($a);

  $a = array(0 => 1, 2, 3, "aba" => 4, -2 => 5, 0 => 6, 7, 8, 9, 10);
  $b = 3.0;

  $c = $a.$b;

  print_r ($a);

  $a = 1.5;

  echo ($a + $b);
  echo "\n";

  echo ($a - $b);
  echo "\n";

  echo ($a * $b);
  echo "\n";

  echo ($a / $b);
  echo "\n";

  echo ($a % $b);
  echo "\n";

  echo ($a += $b);
  echo "\n";

  echo ($a -= $b);
  echo "\n";

  echo ($a *= $b);
  echo "\n";

  echo ($a /= $b);
  echo "\n";

  echo ($a %= $b);
  echo "\n";

  list ($a) = array(1,2);
  echo "A($a)";

  $a = array (1);
  print_r ($a);
  $a = array (1=>5);
  print_r ($a);
  $a = array (2, 3);
  print_r ($a);
  $a = array (2, 7=>5, 3);
  print_r ($a);
  $a = array (1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
  print_r ($a);
  $a = array (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
  print_r ($a);

  /**
   * @param mixed $item
   * @param mixed $depth
   * @return mixed[]
   */
  function bottomUpTree($item, $depth)
  {
     if (!$depth) return array(null,null,$item);
     $item2 = $item + $item;
     $depth--;
     return array(
        bottomUpTree($item2-1,$depth),
        bottomUpTree($item2,$depth),
        $item);
  }

  /**
   * @param mixed $treeNode
   * @return mixed
   */
  function itemCheck($treeNode) {
     return $treeNode[2]
        + ($treeNode[0][0] == null ? itemCheck($treeNode[0]) : $treeNode[0][2])
        - ($treeNode[1][0] == null ? itemCheck($treeNode[1]) : $treeNode[1][2]);
  }
  
  $minDepth = 4;

  $n = 8;
  if ($minDepth + 2 > $n) {
    $maxDepth = $minDepth + 2;
  } else {
    $maxDepth = $n;
  }
  $stretchDepth = $maxDepth + 1;

  $stretchTree = bottomUpTree(0, $stretchDepth);
  $tmp = itemCheck($stretchTree);
  echo ("stretch tree of depth {$stretchDepth}\t check: $tmp\n");
  unset($stretchTree);

  $longLivedTree = bottomUpTree(0, $maxDepth);

  $one = 1;
  $iterations = $one << ($maxDepth);
  do
  {
     $check = 0;
     for($i = 1; $i <= $iterations; ++$i)
     {
        $t = bottomUpTree($i, $minDepth);
        $check = $check + itemCheck($t);
        unset($t);
        $t = bottomUpTree(-$i, $minDepth);
        $check = $check + itemCheck($t);
        unset($t);
     }


     $tmp = $iterations<<1;
     echo("$tmp\t trees of depth $minDepth\t check: $check\n");

     $minDepth = $minDepth + 2;
     $iterations = $iterations >> 2;
  }
  while($minDepth <= $maxDepth);

  $tmp = itemCheck($longLivedTree);
  echo ("long lived tree of depth {$stretchDepth}\t check: $tmp\n");

  //printf("long lived tree of depth %d\t check: %d\n",
  //$maxDepth, itemCheck($longLivedTree)); 
