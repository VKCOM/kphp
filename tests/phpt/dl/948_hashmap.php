@ok benchmar k2_skip
<?php

function test_base_str ($keys, $n) {
  $a = array();
  for ($i = 0; $i < $n; $i++) {
    foreach ($keys as $key) {
      $a[$key] += 1;
    }
  }

  $check = array_slice ($a, 2, 7);
  var_dump ($check);
}

function test_base_int ($keys, $n) {
  $a = array();
  for ($i = 0; $i < $n; $i++) {
    foreach ($keys as $key) {
      ++$a[$key];
    }
  }

  $check = array_slice ($a, 2, 7);
  var_dump ($check);
  //var_dump (memory_get_usage());
}

function gen ($n) {
  if ($n == 0) {
    return array ("");
  }
  $old = gen ($n - 1);
  $res = array();
  foreach ($old as $x) {
    $res[] = $x . "a";
  }
  foreach ($old as $x) {
    $res[] = $x . "b";
  }
  return $res;
}


function gen_int ($n) {
  $res = array();
  $checksum = 0;
  for ($i = 0; $i < $n; $i++) {
    $x = (((987654321 * $i) & 0x7FFFFFFF) * $i) & 0x7FFFFFFF;
    $res[]= $x;
    $checksum += $x;
  }
  printf ("%x\n", $checksum & 0x7FFFFFFF);
  return $res;
}

//$small_keys = array ("a", "hell", "user_id", "something stupid", "small_key", "photo_location_id", "uahash");
//test_base_str ($small_keys, 100000);

//$str_keys = gen (7);
//test_base_str ($small_keys, 20000);


$keys = gen_int (10000);
test_base_int ($keys, 2000);


