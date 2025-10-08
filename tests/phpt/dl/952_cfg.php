@ok
<?
function apiWrapObject($key, $fields = array()) {
return array('_' => $key) + $fields;
}

$v = true;
$v = array(1, 2, 3);
var_dump (apiWrapObject ("abacaba", array (1, 2, 3)));
var_dump (apiWrapObject ("abacaba", $v));
var_dump (apiWrapObject ("abacaba"));
var_dump ($v['_'] == '23');


function test1() {
  $a = 1;
  var_dump ($a);
  $a = "hello world";
  var_dump ($a);
}

function test2() {
  $a = 1;
  var_dump ($a);
  if (true) {
    $a = "hello world";
  } else {
    $a = "bye world";
  }
  var_dump ($a);
}

function test3() {
  $a = 0;
  for ($a = 1; $a < 10; $a++) {
    $tmp = $a;
    $a = "hello";
    $a = $tmp;
  }

  $a = 0;
  while ($a < 10) {
    $tmp = $a;
    $a = "hello";
    $a = $tmp;
    $a++;
  }

  do {
    $tmp = $a;
    $a = "hello";
    $a = $tmp;
    $a++;
  } while ($a < 10);

  $a = 0;

  switch ($a) {
    case 0:
      $a = 1;
      var_dump ($a);
    case 1:
      var_dump ($a);
      break;
    case 2:
      $a = 1;
      var_dump ($a);
      break;
    case 3:
      var_dump ($a);
  }
  $a = 0;
}

function test4($a = 123) {
  var_dump ($a);
  $a = "hello";
  var_dump ($a);
  $a = 123;
  var_dump ($a);
}

function test5() {
  $keys["a"] = "b";

  foreach ($keys as $key => $value) {
    $keys;
  }

}

function geoComputeGeoNum($longitude_degrees, $latitude_degrees) {
  $longitude_degrees = floatval($longitude_degrees);
  $latitude_degrees = floatval($latitude_degrees);
  if ($latitude_degrees > 90 || $latitude_degrees < -90 || $longitude_degrees < -180 || $longitude_degrees > 360) {
    return false;
  }
  $theta = $longitude_degrees * M_PI / 180;
  $phi = $latitude_degrees * M_PI / 180;
  $h = 1;
  if ($phi < 0) {
    $phi = -$phi;
    $h = 0;
  }
  $f = tan(M_PI / 4 - $phi / 2);
  $x = $f * cos($theta);
  $y = $f * sin($theta);
  $x = intval(($x + 1) * 0x1000000);
  $y = intval(($y + 1) * 0x1000000);
  if ($x == 0x2000000) {
    $x--;
  }
  if ($y == 0x2000000) {
    $y--;
  }
  $q = 0;
  for ($i = 1; $i < 0x2000000; $i *= 2) {
    $q += ($x & 1) + ($y & 1) * 2;
    $x >>= 1;
    $y >>= 1;
    $q *= 1 / 4;
  }
  $q = (($q + $h) * 1 / 4) + 1 / 2;
  return intval ($q * 10000);
}


// test5.5. do not put it into function
$x = 0;
switch (0) {
  case 0:
    $x = 123;
}

var_dump ($x);

function test6() {
  $arr = array (1, 2, false);
  echo geoComputeGeoNum ($arr[0], $arr[1]);
  echo "\n";
}

function f(&$x) {
  $x = 123;
}
$x = 321;
echo $x;
echo "\n";
f($x);
echo $x;
echo "\n";

function convertToJSON ($response) {
  return "json";
}
function test7() {
  $response = "hello";
  switch (1) {
    default:
      convertToJSON($response);
      //$response;
  }

}


function test8() {
  switch (1) {
  default:
  case 1:
    echo "A";
    break 1;
  }

  do {
    break;
  } while (false);
}

function test9() {
  do {
    do {
      break 2;
      echo "WA";
    } while (0);
    echo "WA";
  } while (0);
}

function test10() {
  $ol = '';
  foreach ([1] as $_) {
    $ol = 'n';
    break;
  }
  echo $ol, "\n";
}

function test11() {
    if (1)
        $a11 = 1;
    else
        return;
    echo $a11, "\n";
}

function test12() {
    $a = 1;
    if (1) return;
    else exit;
    echo $bbb; // not reachable => no error about uninited
}

function test13() {
    do {
        $ch = 0;
        if (!$ch) break;
        echo $ch;
    } while($ch);
}


test1();
test2();
test3();
test4();
test5();
test6();
test7();
test8();
test9();
test10();
test11();
test12();
test13();
