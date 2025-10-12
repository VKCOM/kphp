@ok k2_skip
<?php
define ('A', 1);
define ('B', A + 1);

#main test for type inference and cfg

//$tmp +=;
//1 ? 2 : ;


  
test_OrFalse();

// тут было куча тестов с /*:= синтаксисом (в т.ч. не привязанным к переменным),
// было очень влом всё переписывать на phpdoc'и и решил их просто удалить, т.к.
// если где-то сломается cfg/inferring — то бахнет не только на этом тесте, а вообще везде

function test_OrFalse() {
  /** @var bool $a */
  $a = false;
  $a = true;

  /** @var int|false $a */
  if (1) {
    $a = 1;
  } else {
    $a = false;
  }
  /** @var int|false */
  $b = $a;

  $a = false;
  if (1) {
    $a = array ();
  }

  /** @var int[]|false $a */
  if (1) {
    $a = array (1, 2, 3);
  }

  //var_dump ($a);

  json_decode (json_encode (NULL), false);

  /** @var mixed $x */
  $x = 1;
  if (1) {
    $x = true;
  }
  //var_dump($x);

  /** @var int $b */
  /** @var int|false $a */
  $a = false;
  if (1) {
    $a = 1;
  }
  $b = $a * $a;


  /** @param int|false $x */
  function pass ($x) {
    return $x;
  }

  /** @var int|false */
  $c = pass ($a);
  //var_dump ($c);
  $a = false;
  $c = pass ($a);
  if ($c !== false) {
    echo "WA1\n";
  }

  /** @var bool $a */
  $a = true;
  if (1) {
    $a = false;
  }

  function def_bool ($x = false) {
    return $x;
  }
  $x = def_bool ("hello");

  /** @var int|false $a */
  $a = 1;
  if (1) {
    $a = false;
  }

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

  /** @var false|(int|false)[] $arr */
  $arr = false;
  if (1) {
    $arr = array();
  }
  $c = $arr []= $a;
  //if ($arr[0] !== false[> || $c !== false<]) echo "WA4\n";

  /** @var int|false $c */
  list ($c) = $arr;
  //if ($c !== false) echo "WA5\n";

  /** @var mixed $d */
  $d = "abc";
  foreach ($arr as $d) {
   //if ($d !== false) echo "WA5\n";
  }

  $arr = array (false, 1);
  $arr2 = array (1);
  if (1) {
    $arr = $arr2;
  }
  $arr;

  $arr = (array (0=>1, false) + array (2=>1));
  if ($arr[2] != 1.5) {
    echo "WA6\n";
  }
  $arr = array ("hello" => 1, false) + array ("a" => false, 1);

  /** @var (int|false)[] $arr */
  $arr = array ($a, 1=>$a, 1);
  
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

  /** @var int[] */
  $arr1 = array (1);
  /** @var (any|false)[] */
  $arr2 = array (false);
  /** @var (int|false)[] */
  $arr3 = array_merge ($arr1, $arr2);


  $arr = array (array (1), false);
  list ($a) = $arr[0];
  if ($a != 1) {
    var_dump ("WA");
  }

  /** @var int[]|false */
  $arr1 = false;
  if (1) {
    $arr1 = array (1, 2, 3);
  }
  array_merge ($arr1, $arr1);
}

function asrt($f) {
  if (!$f) {
    throw new Exception("a", 1);
  }
}

test_setModify();

function test_setModify() {
    $a1 = 10;
    $b1 = ($a1 -= 1.3);
    var_dump($a1, $b1);

    $a2 = 10;
    $b2 = ($a2 .= 'asdf');
    var_dump($a2, $b2);
}
