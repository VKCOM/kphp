@ok
<?php
define ('A', 1);
define ('B', A + 1);

#main test for type inference and cfg

//$tmp +=;
//1 ? 2 : ;


  
test1();
test2();
//test3();
test4(array ("settings" => 12345, "INACTIVE" => false));
//test5();
test_OrFalse();
test6();
test7();
test8(1, array ("abacaba"));
test9();
test10();
test11(array(1));
//test12();
//test13();
test14();
//test15();
test16();
test17();
test18();
test19();

function test_OrFalse() {
  $a = false;
  $a = true;
  $a /*:= bool*/;

  if (1) {
    $a = 1;
  } else {
    $a = false;
  }
  $a /*:= OrFalse < int >*/;
  $b = $a;
  $b /*:= OrFalse < int >*/;
        
  $a = false;
  if (1) {
    $a = array ();
  }

  if (1) {
    $a = array (1, 2, 3);
  }

  //var_dump ($a);

  $a /*:= OrFalse < array <int> >*/;

  json_decode (json_encode (NULL), false);
  
  $x = 1;
  if (1) {
    $x = true;
  }
  $x /*:= var*/;
  //var_dump($x);

  $a = false;
  if (1) {
    $a = 1;
  }
  $b = $a * $a;
  $b /*:= int*/;


  function pass ($x) {
    $x /*:= OrFalse < int >*/;
    return $x;
  }

  $c = pass ($a);
  $c /*:= OrFalse < int >*/;
  //var_dump ($c);
  $a = false;
  $c = pass ($a);
  if ($c !== false) {
    echo "WA1\n";
  }

  $a = true;
  if (1) {
    $a = false;
  }
  $a /*:= bool*/;

  function def_bool ($x = false) {
    return $x;
  }
  $x = def_bool ("hello");

  $a = 1;
  if (1) {
    $a = false;
  }
  $a /*:= OrFalse < int >*/;

  $arr = false;
  if (1) {
    $arr = array();
  }
  $arr[0] = $a;
  if ($arr[0] !== false) echo "WA2\n";

  $arr = false;
  if (1) {
    $arr = array();
  }
  $arr []= $a;
  if ($arr[0] !== false) echo "WA3\n";

  $arr = false;
  if (1) {
    $arr = array();
  }
  $c = $arr []= $a;
  //if ($arr[0] !== false[> || $c !== false<]) echo "WA4\n";
  $arr /*:= OrFalse < array <OrFalse <int > > >*/;

  list ($c) = $arr;
  $c /*:= OrFalse < int >*/;
  //if ($c !== false) echo "WA5\n";

  $d = "abc";
  foreach ($arr as $d) {
   //if ($d !== false) echo "WA5\n";
  }
  $d /*:= var*/;//foreach might not change $d

  $arr = array (false, 1);
  $arr2 = array (1);
  if (1) {
    $arr = $arr2;
  }
  $arr;

  $arr = (array (0=>1, false) + array (2=>1.5));
  if ($arr[2] != 1.5) {
    echo "WA6\n";
  }
  $arr = array ("hello", false) + array ("a", 1);
  
  $arr = array ($a, 1=>$a, 1);
  $arr /*:= array <OrFalse < int > >*/;
  
  //if ($arr[0] !== false) {
    //echo "WA7\n";
  //}
  //if ($arr[1] !== false) {
    //echo "WA8\n";
  //}
  //if ($arr[2] !== false) {
    ////echo "WA8\n";
  //}


  $t = false;
  if (0) {
    $t = 1;
  }
  //var_dump ($t);

  $arr1 = array (1);
  $arr1 /*:= array <int>*/;
  $arr2 = array (false);
  $arr2 /*:= array <OrFalse < Unknown > >*/;
  $arr3 = array_merge ($arr1, $arr2);
  $arr3 /*:= array <OrFalse < int > >*/;


  $arr = array (array (1), false);
  list ($a) = $arr[0];
  if ($a != 1) {
    var_dump ("WA");
  }

  $arr1 = false;
  if (1) {
    $arr1 = array (1, 2, 3);
  }
  $arr1 /*:= OrFalse <array <int> >*/;
  array_merge ($arr1, $arr1);
}

function asrt($f) {
  if (!$f) {
    throw new Exception("a", 1);
  }
}

/**
 * @kphp-required
 */
function odd ($x) {
    return $x & 1;
}
//one set
function test1() {

  //$a /*:= Unknown*/;
  
  $a = 1;
  $a /*:= int*/;

  $a = 1.0;
  $a /*:= float*/;
  
  $a = "1";
  $a /*:= string*/;
  
  $a = false;
  $a /*:= OrFalse < Unknown >*/;
  
  if (1) {
    $a = 1;
  } else {
    $a = "";
  }

  $a /*:= var*/;

  $a = array(1, 2, 3);
  $a /*:= array <int>*/;

  $a = array (1, "");
  $a /*:= array <var>*/;

  $a = array();
  $a /*:= array <Unknown>*/;

  if (1) {
    $a = array();
  } else {
    $a = array (1);
  }
  $a /*:= array <int>*/;

  $a = array (1, 1.0);
  $a /*:= array <float>*/;

  $array = array(213);
  shuffle ($array);
  $array /*:= array <int>*/;

  $array = array (1, 2, 3, 4, 5);
  $array /*:= array <int>*/;
  
  
  
  array_map ('odd', $array);



  $tt = array_map ('odd', $array);
  $array = $tt;




  $array /*:= array <int>*/;
 

  1 + 1 /*:= int*/;
  1.0 + 1 /*:= float*/;
  1000000 * 1000000 /*:= int*/;
  "1000000" * 1000000 /*:= var*/;
  asrt ("100" * 100 == 10000);

  5 / 1 /*:= float*/;

  $a = array (1);
  $a /*:= array <int>*/;

  if (1) {
    $a = array();
  }
  $a /*:= array <int>*/;

  function def_empty_arr ($x = array()) {
    return $x;
  }
  $x = def_empty_arr();
  $x /*:= array <int>*/;
  def_empty_arr(array (1, 2, 3));


  $a = min (array (1, 2, 3));
  $a /*:= int*/;
  $a = min (1, 1.5);
  $a /*:= float*/;
  $a = max (false, "str");

  $a = intval ("123");
  $a /*:= int*/;
  $a = (array) 1;
  $a = "asdf";

  $a = array (array (array (1)), array());

  /**
   * @kphp-disable-warnings return
   */
  function return_op_empty() {
    if (1) {
      return 1;
    }
  }
  $x = return_op_empty();
  $x /*:= var*/;


  //Warning on the next line
  $x = isset ($ttt);
  $x /*:= bool*/;
}


function test2 () {
  $photo = array("sizes" => array());
  if (0) {
    $photo = "Asda";
  }

  $kid = $photo['kid'];
  $sizes = $photo['sizes'];
  $photo_hash = $photo['photo'];
  list(,$max_size) = explode(':', $photo_hash);
  $max_size = array(1);
  $max_size = $max_size[0];
  
  foreach ($sizes as $size_data) {
    if ($size_data[0] == $max_size) 
    {
      $size_x = $size_data[4];
      $size_y = $size_data[5];
      break;
    }
 
  }

  if ($size_x && $size_y) {
    $fields['width'] = $size_x;
    $fields['height'] = $size_y;
    $fields['size_mask'] = 15; // has sizes 'o', 'p', 'q', 'r'
  }
}

function test3 ($a = array()) {
  $b = array (1);

  $b[] = $a[0];
}

function positive ($x) {
  $x = intval ($x);
  return $x > 0 ? $x : 0;
}

function test4($member) {
  if ($member['INACTIVE']) {
    return true;
  }
  $setting_bits = positive($member['settings']);
  $config[1][2] = 3;
  return ($setting_bits & $config['member_settings']['inactive']) != 0;
}

function test5() {
  $x = "";
  if (0) {
    $x[] = 1;
  }
  $arr1[] = $x;
  $arr2 = array ("");
  var_dump ($arr1 === $arr1);
  var_dump ($arr1 === $arr2);

  $arr3 = $arr1;
  if (0) {
    $arr3 = false;
  }
  var_dump ($arr3 === $arr2);
  var_dump ($arr1 === $arr3);
  var_dump ($arr3 === $arr3);

  unset($GlobalContacts[1]);
}

function test6() {
  $result = array();
  static $flex_langs = array();
  //$result['_'];
  //var_dump ($result['_']);
}

function test7($val = B) {
  if ($val != 2) {
    echo "WA9\n";
  }

  $a = A;
  $a /*:= int*/;

  $b = B;
  $b /*:= int*/;
}

function test8($mid, $sid_data) {
  //global $GlobalDisablePwdReplica;

  //$mid = positive ($mid);
  //if ($mid != $sid_data['id'] || !strlen($sid_data['hash']) || !strlen($sid_data['hash_name'])) return false;

  //$mc_key = "check_hash{$mid},0:{$sid_data['hash']}={$sid_data['hash_name']}";

  //$MC_Pwd = new Memcache;
  //$res = $MC_Pwd ? $MC_Pwd->get(array($mc_key)) : array();
  //if (!is_array($res) || !isset($res[$mc_key])) {
  //  $MC_PwdRep = new Memcache;
  //  $MC_PwdSel = $MC_PwdRep ? $MC_PwdRep : $MC_Pwd;
  //  $res = $MC_PwdSel ? $MC_PwdSel->get(array($mc_key)) : array();
  //  if (!is_array($res) || !isset($res[$mc_key])) {
  //    return null;
  //  }
  //}
  //$res = "{$res[$mc_key]}";

  //if ($res === '7' || $res === '4') {
  //  if ($res === '4') {
  //    // must block user - current pwd is compromised
  //  }
  //  return true;
  //}
  //return false;
}

function test9() {
  $MC = new Memcache;
  $MC = $MC ? $MC : $MC;
  $XMC = false ? $MC : false;
}

function test10() {
  if (true) {
    return new Memcache;
  } else {
    return false;
  }
}

function test11($keys) {
  $result = false;
  is_array ($keys);

  foreach ($keys as $key) {
    $result = true;
  }
  return $result;
}

function f($x) {
}
function test13() {
  $xx = 123;
  switch ("hello") {
  case "A":
    f($tmp = 1);
    //$xx = $tmp;
    //$tmp /*:= int*/;
    //$kludges = array ("attach1_type" => "audio");
    //for ($i = 1; $type = $kludges['attach'.$i.'_type']; ++$i) {
    //  if ($type == 'audio' && $kludges['attach'.$i]) {
    //    $audio_ids[] = $kludges['attach'.$i];
    //  }
    //}
    //break;
  case "B":
    f ($tmp = "str");
    $xx = $tmp;
    $tmp /*:= string*/;
    $kludges = array ("attach1_type" => "audio");
    for ($i = 1; $type = $kludges['attach'.$i.'_type']; ++$i) {
      if ($type == 'audio' && $kludges['attach'.$i]) {
        $audio_ids[] = $kludges['attach'.$i];
      }
    }
    break;
  }

  $kludges;
}

function test14() {
  $a = array();
  array_unshift ($a, 1);
  $a /*:= array <int>*/;

  $a = array();
  array_push ($a, 1, 1.0);
  $a /*:= array <float>*/;

  //function return_void() {
  //  $v = 5; 
  //}

  //$b = return_void();

  function return_xx() {
    if (1) {
      return 1;
    } else {
      return ceil (1 / 2);
    }
  }
  $b = return_xx();
}


function test16() {
  $a = 1;
  isset ($a);
  $a /*:= var*/;

  $a = array (array(1, 2, 3));
  is_array ($a[1]);
  $a /*:= array <var>*/;

  $a = array (1, 2, 3);
  isset ($a[1]);
  $a /*:= array <int>*/;


  //$a = array (1, 2, 3);
  //$b = $a[1];
  //$c = $b;
  //isset ($c);
  //$c /*:= var*/;

  function get_var16(){
    $a = array (1);
    if (0) {
      return "abc";
    }
    return $a[1];
  }
  $b = get_var16();
  //Warning on the next line
  isset ($b);
  $b /*:= var*/;
  function test($a) { 
    if ($a == 1) 
      return array('test'); 
    $arr = array(100); 
    return $arr[2]; 
  } 
  $v = test(2); 
  //Warning on the next line
  isset($v);

  $a = array ("hest");
  $b = $a[1];
  is_numeric ($b);


  $a = array(1);
  $b = 123;
  $c = $a[1];
  function test16_ref (&$x) {
  }
  test16_ref($c);
  test16_ref($b);
  //No warnings on the next line
  isset ($b);


  $a = array (1);
  $b = $a[1];
  $c = $b;
  if ($c === false) {}
  $c /*:= var*/;

  function check_var16($x) {
    is_array ($x);
  }
  if (0) {
    $x = array();
  }
  check_var16 ($x);

}


/** test extern_function **/
////extern_function ef1;
////function ef1 ($v) {
////  $v /*:= var*/;
////}

//extern_function ef2($v);
//function ef2 ($v) {
  //$v /*:= var*/;
  //return 1;
//}
//extern_function ef3 ($v ::: float) ::: array <var>;
//function ef3 ($v) {
  //$v /*:= float*/;
  //return array (1, 2, 3);
//}

//extern_function ef4 (...);
//function ef4 ($a, $b) {
  //echo $a;
  //echo $b;
//}

//function test12() {
  ////$res = ef1();
  ////$res /*:= var*/;
  //$res = ef2("str");
  //$res /*:= var*/;
  //$res = ef3("str");
  //$res /*:= array <var>*/;

  //ef4 (1, "t");
//}

//function test15() {
//  if (1) {
//    return;
//    $a = 5;
//  }
//  $a = 6;
//  return $a;
//}


function test17() {
  if (1) {
    $bad = 1;
  }
}

function test18() {
  function f18($x) {
    is_array ($x);
  }
  try {
    $a = 1;
    $a /*:= int*/;
    $a = true;
    $a /*:= bool*/;
    $a = array(1);
    $a /*:= array <int>*/;
    f18($a);
  } catch (Exception $e) {
    echo "exception\n";
  }
  try {
  } catch (Exception $e) {
    var_dump ("test");
  }
}

function test19() {
  $a = array ( array (1.0));
  foreach ($a as &$e) {
  }
  $a /*:= array <array <float> >*/;
}

function test20() {
  array_merge (array (1), array ("a"));
}

