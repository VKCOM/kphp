@ok
<?php
require_once 'kphp_tester_include.php';

function printAllOfArray($a) {
  var_dump(array_first_key($a));
  var_dump(array_key_first($a));
  var_dump(array_first_value($a));
  var_dump(array_last_key($a));
  var_dump(array_key_last($a));
  var_dump(array_last_value($a));
  echo "\n";
}

printAllOfArray([1,2,3]);
printAllOfArray(['1','2','3']);
printAllOfArray(['one' => 1, 'two' => 2, 'three' => 3]);

$var1 = false ? 4 : [1,'2',['one'=>1, 'two'=>'2']];
printAllOfArray($var1);
printAllOfArray($var1[2]);

class AA {
  var $a = 0;
  function __construct($a) { $this->a = $a; }
}

$arr_of_inst = [new AA(1), new AA(2)];
var_dump(array_first_key($arr_of_inst));
var_dump(array_key_first($arr_of_inst));
var_dump(array_last_key($arr_of_inst));
var_dump(array_key_last($arr_of_inst));
$inst = array_first_value($arr_of_inst);
var_dump($inst->a);
unset($arr_of_inst[0]);
var_dump(array_first_key($arr_of_inst));
var_dump(array_key_first($arr_of_inst));
var_dump(array_last_key($arr_of_inst));
var_dump(array_key_last($arr_of_inst));
$inst = array_first_value($arr_of_inst);
var_dump($inst->a);
