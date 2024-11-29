@ok
<?php
require_once 'kphp_tester_include.php';


/** @kphp-required */
function generate_aa() {
    $inner = new Classes\LoggingLikeArray([404]);
    $outter = new Classes\LoggingLikeArray([ to_mixed($inner) ]);
    return to_mixed($outter);
}

function test() {  
  /** @var callable */
  $x = "generate_aa";
  $arr = [$x];

  var_dump(empty($arr[0]()[0][0]));
  var_dump(empty($arr[0]()[1][0]));

  var_dump(isset($arr[0]()[1][0]));
  var_dump(isset($arr[0]()[0][0]));

  var_dump($arr[0]()[1][0]);
  var_dump($arr[0]()[0][0]);
}

test();
