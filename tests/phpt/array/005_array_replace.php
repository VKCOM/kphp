@ok
<?php
require_once 'kphp_tester_include.php';
// based on https://github.com/php/php-src/blob/master/ext/standard/tests/array/array_replace.phpt

function base_array_replace_test()
{
  $array1 = array(
    0 => 'dontclobber',
    '1' => 'unclobbered',
    'test2' => 0.0,
    'test3' => array(
      'testarray2' => true,
      1 => array(
        'testsubarray1' => 'dontclobber2',
        'testsubarray2' => 'dontclobber3',
      ),
    ),
  );
  $array2 = array(
    1 => 'clobbered',
    'test3' => array(
      'testarray2' => false,
    ),
    'test4' => array(
      'clobbered3' => array(0, 1, 2),
    ),
  );
  $data = array_replace($array1, $array2);
  var_dump($data);
  $data1 = array_replace($array1, $array2, $array1);
  var_dump($data1);
  $data2 = array_replace($array1);
  var_dump($data2);
  $data3 = array_replace($array1, $array2, $array1, $array2, $array1);
  var_dump($data3);
}

function empty_array_array_replace_test()
{
  $a = array(array());
  $r = array(array());
  var_dump(array_replace($a, $r));
}

function same_types_array_replace_test()
{
  var_dump(array_replace(["qwerty", "!", "..."], ["hello", "world"]));
}

function different_types_array_replace_test()
{
  var_dump(array_replace([1, 2, 3], ["hello", "world"]));
}

function instances_array_replace_test()
{
  interface my_interface {};
  class A implements my_interface{ public $a = 1; };
  class B implements my_interface{ public $b = 2; };
  class C implements my_interface{ public $c = 3; };
  $arr = array_replace(array(new A, new B, new C), array(new C, new A, new B));
  foreach ($arr as $item) {
    var_dump(to_array_debug($item));
  }
}

function null_array_replace_test() {
  var_dump(array_replace(array(1, 2, 3), array(null, 5, null)));
}

function vector_to_map_array_replace_test() {
  var_dump(array_replace(array(1, 2, 3), array("key" => "value")));
}

function map_array_replace_test() {
  var_dump(array_replace(array("key1" => "value1", "key2" => "value2", "key3" => "value3"), array("key1" => "value", 0 => 123)));
}

base_array_replace_test();
empty_array_array_replace_test();
same_types_array_replace_test();
different_types_array_replace_test();
instances_array_replace_test();
null_array_replace_test();
vector_to_map_array_replace_test();
map_array_replace_test();
