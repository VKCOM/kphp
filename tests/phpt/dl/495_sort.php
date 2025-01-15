@ok benchmark callback
<?php
echo "*** Testing sort() : basic functionality ***\n";

$tmp = 1 << 24;
$data = array( 
	'PHP',
	17=>'PHP: Hypertext Preprocessor',
	5=>'Test',
	'test'=>27,
	1000=>'test',
	"-1000"=>array('banana', 'orange'),
	'monkey',
	$tmp=>-0.333333333
);

/**
 * @kphp-required
 */
function cmp ($a, $b) {
    is_array ($a)
        and $a = array_sum ($a);
    is_array ($b)
        and $b = array_sum ($b);
    return strcmp ($a, $b);
}

echo " -- Testing uasort() -- \n";
uasort ($data, 'cmp');
var_dump ($data);


echo "\n -- Testing uksort() -- \n";
uksort ($data, 'cmp');
var_dump ($data);

echo "\n -- Testing usort() -- \n";
usort ($data, 'cmp');
var_dump ($data);

$n = 300001;
$to_sort = array();
for ($i = 0; $i < $n; $i++) {
  $to_sort[rand(0, 1000000000)] = rand(0, 1000000000);
}

asort ($to_sort);

krsort ($to_sort);

sort ($to_sort);

// associative array containing unsorted string values  
$unsorted_strings =   array( "l" => "lemon", "o" => "orange", "b" => "banana" );
 
// array with default keys containing unsorted numeric values
$unsorted_numerics =  array( 100, 33, 555, 22 );

echo "\n-- Testing sort() by supplying string array, 'flag' value is default --\n";
$temp_array = $unsorted_strings;
sort($temp_array);
var_dump( $temp_array);

echo "\n-- Testing sort() by supplying numeric array, 'flag' value is default --\n";
$temp_array = $unsorted_numerics;
var_dump($temp_array);
sort($temp_array);
var_dump($temp_array);

echo "\n-- Testing sort() by supplying string array, 'flag' = SORT_REGULAR --\n";
$temp_array = $unsorted_strings;
sort($temp_array, SORT_REGULAR);
var_dump( $temp_array);

echo "\n-- Testing sort() by supplying numeric array, 'flag' = SORT_REGULAR --\n";
$temp_array = $unsorted_numerics;
sort($temp_array, SORT_REGULAR);
var_dump( $temp_array);

echo "\n-- Testing sort() by supplying string array, 'flag' = SORT_STRING --\n";
$temp_array = $unsorted_strings;
sort($temp_array, SORT_STRING);
var_dump( $temp_array);

echo "\n-- Testing sort() by supplying numeric array, 'flag' = SORT_NUMERIC --\n";
$temp_array = $unsorted_numerics;
sort($temp_array, SORT_NUMERIC);
var_dump( $temp_array);

echo "Done\n";

echo "*** Testing sort() : usage variations ***\n";

// an array containing unsorted octal values
$unsorted_oct_array = array(01235, 0321, 0345, 066, 0772, 077, -066, -0345, 0);

echo "\n-- Testing sort() by supplying octal value array, 'flag' value is default  --\n";
$temp_array = $unsorted_oct_array;
sort($temp_array);
var_dump($temp_array);

echo "\n-- Testing sort() by supplying octal value array, 'flag' value is SORT_REGULAR  --\n";
$temp_array = $unsorted_oct_array;
sort($temp_array, SORT_REGULAR);
var_dump($temp_array);

echo "\n-- Testing sort() by supplying octal value array, 'flag' value is SORT_NUMERIC  --\n";
$temp_array = $unsorted_oct_array;
sort($temp_array, SORT_NUMERIC);
var_dump($temp_array);


$various_arrays = array (
  // negative/posative integers array
  array(11, -11, 21, -21, 31, -31, 0, 41, -41),

  // float value array
//  array(10.5, -10.5, 10.5e2, 10.6E-2, .5, .01, -.1),

  // mixed value array
//  array(.0001, .0021, -.01, -1, 0, .09, 2, -.9, 10.6E-2, -10.6E-2, 33),
 
  // array values contains minimum and maximum ranges
//  array(2147483647, 2147483648, -2147483647, -2147483648, -0, 0, -2147483649),
 
  // array values contains minimum and maximum ranges
  array(2147483647, 2147483647, -2147483647, -2147483647, -0, 0, -2147483648)
);

// set of possible flag values
$flag_value = array("SORT_REGULAR" => SORT_REGULAR, "SORT_NUMERIC" => SORT_NUMERIC);

$count = 1;
echo "\n-- Testing sort() by supplying various integer/float arrays --\n";

// loop through to test sort() with different arrays
foreach ($various_arrays as $array) {
  echo "\n-- Iteration $count --\n";

  echo "- With Default sort flag -\n";
  $temp_array = $array; 
  sort($temp_array);
  var_dump($temp_array);

  // loop through $flag_value array and setting all possible flag values
  foreach($flag_value as $key => $flag){
    echo "- Sort flag = $key -\n";
    $temp_array = $array; 
    sort($temp_array, $flag);
    var_dump($temp_array);
  }  
  $count++;
} 

$value1 = 100;
$value2 = 33;
$value3 = 555;

// an array containing integer references 
$unsorted_numerics =  array( $value1 , $value2, $value3);

echo "\n-- Testing sort() by supplying reference variable array, 'flag' value is default --\n";
$temp_array = $unsorted_numerics;
sort($temp_array);
var_dump( $temp_array);

echo "\n-- Testing sort() by supplying reference variable array, 'flag' = SORT_REGULAR --\n";
$temp_array = $unsorted_numerics;
sort($temp_array, SORT_REGULAR);
var_dump( $temp_array);

echo "\n-- Testing sort() by supplying reference variable array, 'flag' = SORT_NUMERIC --\n";
$temp_array = $unsorted_numerics;
sort($temp_array, SORT_NUMERIC);
var_dump( $temp_array);

echo "Done\n";

$various_arrays = array (
  // group of escape sequences 
  array("\a", "\cx", "\e", "\f", "\n", "\r", "\t", "\xhh", "\ddd", "\v"),

  // array contains combination of capital/small letters 
  array("lemoN", "Orange", "banana", "apple", "Test", "TTTT", "ttt", "ww", "x", "X", "oraNGe", "BANANA")
);

$flags = array("SORT_REGULAR" => SORT_REGULAR, "SORT_STRING" => SORT_STRING);

$count = 1;
echo "\n-- Testing sort() by supplying various string arrays --\n";

// loop through to test sort() with different arrays
foreach ($various_arrays as $array) {
  echo "\n-- Iteration $count --\n";

  echo "- With Default sort flag -\n";
  $temp_array = $array;
  sort($temp_array);
//  var_dump($temp_array);

  // loop through $flags array and setting all possible flag values
  foreach($flags as $key => $flag){
    echo "- Sort flag = $key -\n";
    $temp_array = $array;
    sort($temp_array, $flag);
//    var_dump($temp_array);
  }
  $count++;
}

$unsorted_hex_array = array(0x1AB, 0xFFF, 0xF, 0xFF, 0x2AA, 0xBB, 0x1ab, 0xff, -0xFF, 0, -0x2aa);

echo "\n-- Testing sort() by supplying hexadecimal value array, 'flag' value is default  --\n";
$temp_array = $unsorted_hex_array;
sort($temp_array);
var_dump($temp_array);

echo "\n-- Testing sort() by supplying hexadecimal value array, 'flag' value is SORT_REGULAR  --\n";
$temp_array = $unsorted_hex_array;
sort($temp_array, SORT_REGULAR);
var_dump($temp_array);

echo "\n-- Testing sort() by supplying hexadecimal value array, 'flag' value is SORT_NUMERIC  --\n";
$temp_array = $unsorted_hex_array;
sort($temp_array, SORT_NUMERIC);
var_dump($temp_array);


// bool value array
$bool_values = array (true, false, TRUE, FALSE);

echo "\n-- Testing sort() by supplying bool value array, 'flag' value is default --\n";
$temp_array = $bool_values;
sort($temp_array);
var_dump($temp_array);

echo "\n-- Testing sort() by supplying bool value array, 'flag' value is SORT_REGULAR --\n";
$temp_array = $bool_values;
sort($temp_array, SORT_REGULAR);
var_dump($temp_array);

echo "\n-- Testing sort() by supplying bool value array, 'flag' value is SORT_NUMERIC  --\n";
$temp_array = $bool_values;
sort($temp_array, SORT_NUMERIC);
var_dump($temp_array);

echo "\n-- Testing sort() by supplying bool value array, 'flag' value is SORT_STRING --\n";
$temp_array = $bool_values;
sort($temp_array, SORT_STRING);
var_dump($temp_array);

echo "Done\n";

$various_arrays = array (
  // null array
  array(),

  // array contains null sub array
  array( array() ),

  // array of arrays along with some values
//  array(44, 11, array(64, 61) ),

  // array containing sub arrays
  array(array(33, -5, 6), array(11), array(22, -55), array() )
);


$count = 1;
echo "\n-- Testing sort() by supplying various arrays containing sub arrays --\n";

// loop through to test sort() with different arrays
foreach ($various_arrays as $array) {
 
  echo "\n-- Iteration $count --\n"; 
  // testing sort() function by supplying different arrays, flag value is default
  echo "- With Default sort flag -\n";
  $temp_array = $array;
  sort($temp_array);
  var_dump($temp_array);

  // testing sort() function by supplying different arrays, flag value = SORT_REGULAR
  echo "- Sort flag = SORT_REGULAR -\n";
  $temp_array = $array;
  sort($temp_array, SORT_REGULAR);
  var_dump($temp_array);
  $count++;
}


$various_arrays = array(
  array(5 => 55, 6 => 66, 2 => 22, 3 => 33, 1 => 11),
  array(1, 1, 8 => 1,  4 => 1, 19, 3 => 13),
  array('bar' => 'baz', "foo" => 1),
);

$count = 1;
echo "\n-- Testing sort() by supplying various arrays with key values --\n";

// loop through to test sort() with different arrays, 
// to test the new keys for the elements in the sorted array 
foreach ($various_arrays as $array) {
  echo "\n-- Iteration $count --\n";

  echo "- With Default sort flag -\n";
  $temp_array = $array;
  sort($temp_array);
  var_dump($temp_array);

  echo "- Sort flag = SORT_REGULAR -\n";
  $temp_array = $array;
  sort($temp_array, SORT_REGULAR);
  var_dump($temp_array);
  $count++;
}










// associative array containing unsorted string values  
$unsorted_strings =   array( "l" => "lemon", "o" => "orange", "b" => "banana" );
 
// array with default keys containing unsorted numeric values
$unsorted_numerics =  array( 100, 33, 555, 22 );

echo "\n-- Testing rsort() by supplying string array, 'flag' value is default --\n";
$temp_array = $unsorted_strings;
rsort($temp_array);
var_dump( $temp_array);

echo "\n-- Testing rsort() by supplying numeric array, 'flag' value is default --\n";
$temp_array = $unsorted_numerics;
rsort($temp_array);
var_dump( $temp_array);

echo "\n-- Testing rsort() by supplying string array, 'flag' = SORT_REGULAR --\n";
$temp_array = $unsorted_strings;
rsort($temp_array, SORT_REGULAR);
var_dump( $temp_array);

echo "\n-- Testing rsort() by supplying numeric array, 'flag' = SORT_REGULAR --\n";
$temp_array = $unsorted_numerics;
rsort($temp_array, SORT_REGULAR);
var_dump( $temp_array);

echo "\n-- Testing rsort() by supplying string array, 'flag' = SORT_STRING --\n";
$temp_array = $unsorted_strings;
rsort($temp_array, SORT_STRING);
var_dump( $temp_array);

echo "\n-- Testing rsort() by supplying numeric array, 'flag' = SORT_NUMERIC --\n";
$temp_array = $unsorted_numerics;
rsort($temp_array, SORT_NUMERIC);
var_dump( $temp_array);

echo "Done\n";

echo "*** Testing rsort() : usage variations ***\n";

// an array containing unsorted octal values
$unsorted_oct_array = array(01235, 0321, 0345, 066, 0772, 077, -066, -0345, 0);

echo "\n-- Testing rsort() by supplying octal value array, 'flag' value is default  --\n";
$temp_array = $unsorted_oct_array;
rsort($temp_array);
var_dump($temp_array);

echo "\n-- Testing rsort() by supplying octal value array, 'flag' value is SORT_REGULAR  --\n";
$temp_array = $unsorted_oct_array;
rsort($temp_array, SORT_REGULAR);
var_dump($temp_array);

echo "\n-- Testing rsort() by supplying octal value array, 'flag' value is SORT_NUMERIC  --\n";
$temp_array = $unsorted_oct_array;
rsort($temp_array, SORT_NUMERIC);
var_dump($temp_array);


$various_arrays = array (
  // negative/posative integers array
  array(11, -11, 21, -21, 31, -31, 0, 41, -41),

  // float value array
//  array(10.5, -10.5, 10.5e2, 10.6E-2, .5, .01, -.1),

  // mixed value array
//  array(.0001, .0021, -.01, -1, 0, .09, 2, -.9, 10.6E-2, -10.6E-2, 33),
 
  // array values contains minimum and maximum ranges
//  array(2147483647, 2147483648, -2147483647, -2147483648, -0, 0, -2147483649),
 
  // array values contains minimum and maximum ranges
  array(2147483647, 2147483647, -2147483647, -2147483647, -0, 0, -2147483648)
);

// set of possible flag values
$flag_value = array("SORT_REGULAR" => SORT_REGULAR, "SORT_NUMERIC" => SORT_NUMERIC);

$count = 1;
echo "\n-- Testing rsort() by supplying various integer/float arrays --\n";

// loop through to test rsort() with different arrays
foreach ($various_arrays as $array) {
  echo "\n-- Iteration $count --\n";

  echo "- With Default rsort flag -\n";
  $temp_array = $array; 
  rsort($temp_array);
  var_dump($temp_array);

  // loop through $flag_value array and setting all possible flag values
  foreach($flag_value as $key => $flag){
    echo "- Sort flag = $key -\n";
    $temp_array = $array; 
    rsort($temp_array, $flag);
    var_dump($temp_array);
  }  
  $count++;
} 

$value1 = 100;
$value2 = 33;
$value3 = 555;

// an array containing integer references 
$unsorted_numerics =  array( $value1 , $value2, $value3);

echo "\n-- Testing rsort() by supplying reference variable array, 'flag' value is default --\n";
$temp_array = $unsorted_numerics;
rsort($temp_array);
var_dump( $temp_array);

echo "\n-- Testing rsort() by supplying reference variable array, 'flag' = SORT_REGULAR --\n";
$temp_array = $unsorted_numerics;
rsort($temp_array, SORT_REGULAR);
var_dump( $temp_array);

echo "\n-- Testing rsort() by supplying reference variable array, 'flag' = SORT_NUMERIC --\n";
$temp_array = $unsorted_numerics;
rsort($temp_array, SORT_NUMERIC);
var_dump( $temp_array);

echo "Done\n";

$various_arrays = array (
  // group of escape sequences 
  array("\a", "\cx", "\e", "\f", "\n", "\r", "\t", "\xhh", "\ddd", "\v"),

  // array contains combination of capital/small letters 
  array("lemoN", "Orange", "banana", "apple", "Test", "TTTT", "ttt", "ww", "x", "X", "oraNGe", "BANANA")
);

$flags = array("SORT_REGULAR" => SORT_REGULAR, "SORT_STRING" => SORT_STRING);

$count = 1;
echo "\n-- Testing rsort() by supplying various string arrays --\n";

// loop through to test rsort() with different arrays
foreach ($various_arrays as $array) {
  echo "\n-- Iteration $count --\n";

  echo "- With Default rsort flag -\n";
  $temp_array = $array;
  rsort($temp_array);
//  var_dump($temp_array);

  // loop through $flags array and setting all possible flag values
  foreach($flags as $key => $flag){
    echo "- Sort flag = $key -\n";
    $temp_array = $array;
    rsort($temp_array, $flag);
//    var_dump($temp_array);
  }
  $count++;
}

$unsorted_hex_array = array(0x1AB, 0xFFF, 0xF, 0xFF, 0x2AA, 0xBB, 0x1ab, 0xff, -0xFF, 0, -0x2aa);

echo "\n-- Testing rsort() by supplying hexadecimal value array, 'flag' value is default  --\n";
$temp_array = $unsorted_hex_array;
rsort($temp_array);
var_dump($temp_array);

echo "\n-- Testing rsort() by supplying hexadecimal value array, 'flag' value is SORT_REGULAR  --\n";
$temp_array = $unsorted_hex_array;
rsort($temp_array, SORT_REGULAR);
var_dump($temp_array);

echo "\n-- Testing rsort() by supplying hexadecimal value array, 'flag' value is SORT_NUMERIC  --\n";
$temp_array = $unsorted_hex_array;
rsort($temp_array, SORT_NUMERIC);
var_dump($temp_array);


// bool value array
$bool_values = array (true, false, TRUE, FALSE);

echo "\n-- Testing rsort() by supplying bool value array, 'flag' value is default --\n";
$temp_array = $bool_values;
rsort($temp_array);
var_dump($temp_array);

echo "\n-- Testing rsort() by supplying bool value array, 'flag' value is SORT_REGULAR --\n";
$temp_array = $bool_values;
rsort($temp_array, SORT_REGULAR);
var_dump($temp_array);

echo "\n-- Testing rsort() by supplying bool value array, 'flag' value is SORT_NUMERIC  --\n";
$temp_array = $bool_values;
rsort($temp_array, SORT_NUMERIC);
var_dump($temp_array);

echo "\n-- Testing rsort() by supplying bool value array, 'flag' value is SORT_STRING --\n";
$temp_array = $bool_values;
rsort($temp_array, SORT_STRING);
var_dump($temp_array);

echo "Done\n";

$various_arrays = array (
  // null array
  array(),

  // array contains null sub array
  array( array() ),

  // array of arrays along with some values
//  array(44, 11, array(64, 61) ),

  // array containing sub arrays
  array(array(33, -5, 6), array(11), array(22, -55), array() )
);


$count = 1;
echo "\n-- Testing rsort() by supplying various arrays containing sub arrays --\n";

// loop through to test rsort() with different arrays
foreach ($various_arrays as $array) {
 
  echo "\n-- Iteration $count --\n"; 
  // testing rsort() function by supplying different arrays, flag value is default
  echo "- With Default rsort flag -\n";
  $temp_array = $array;
  rsort($temp_array);
  var_dump($temp_array);

  // testing rsort() function by supplying different arrays, flag value = SORT_REGULAR
  echo "- Sort flag = SORT_REGULAR -\n";
  $temp_array = $array;
  rsort($temp_array, SORT_REGULAR);
  var_dump($temp_array);
  $count++;
}


$various_arrays = array(
  array(5 => 55, 6 => 66, 2 => 22, 3 => 33, 1 => 11),
  array(1, 1, 8 => 1,  4 => 1, 19, 3 => 13),
  array('bar' => 'baz', "foo" => 1),
);

$count = 1;
echo "\n-- Testing rsort() by supplying various arrays with key values --\n";

// loop through to test rsort() with different arrays, 
// to test the new keys for the elements in the sorted array 
foreach ($various_arrays as $array) {
  echo "\n-- Iteration $count --\n";

  echo "- With Default rsort flag -\n";
  $temp_array = $array;
  rsort($temp_array);
  var_dump($temp_array);

  echo "- Sort flag = SORT_REGULAR -\n";
  $temp_array = $array;
  rsort($temp_array, SORT_REGULAR);
  var_dump($temp_array);
  $count++;
}













$tmp = 1 << 24;
$data = array( 
	17=>1212,
	5=>123123,
	'test'=>27,
	1000=>234,
	"-1000"=>"5",
	7.300000001,
	$tmp=>-0.333333333
);

echo "Unsorted data:\n";
var_dump ($data);

    echo "\n -- Testing asort() -- \n";
	echo "No second argument:\n";
    asort ($data);
    var_dump ($data);
	echo "Using SORT_REGULAR:\n";
    asort ($data, SORT_REGULAR);
    var_dump ($data);
	echo "Using SORT_NUMERIC:\n";
    asort ($data, SORT_NUMERIC);
    var_dump ($data);
	echo "Using SORT_STRING\n";
    asort ($data, SORT_STRING);
    var_dump ($data);


    echo "\n -- Testing arsort() -- \n";
	echo "No second argument:\n";
    arsort ($data);
    var_dump ($data);
	echo "Using SORT_REGULAR:\n";
    arsort ($data, SORT_REGULAR);
    var_dump ($data);
	echo "Using SORT_NUMERIC:\n";
    arsort ($data, SORT_NUMERIC);
    var_dump ($data);
	echo "Using SORT_STRING\n";
    arsort ($data, SORT_STRING);
    var_dump ($data);


    echo "\n -- Testing ksort() -- \n";
	echo "No second argument:\n";
    ksort ($data);
    var_dump ($data);
	echo "Using SORT_REGULAR:\n";
    ksort ($data, SORT_REGULAR);
    var_dump ($data);
	echo "Using SORT_NUMERIC:\n";
    ksort ($data, SORT_NUMERIC);
    var_dump ($data);
	echo "Using SORT_STRING\n";
    ksort ($data, SORT_STRING);
    var_dump ($data);


    echo "\n -- Testing krsort() -- \n";
	echo "No second argument:\n";
    krsort ($data);
    var_dump ($data);
	echo "Using SORT_REGULAR:\n";
    krsort ($data, SORT_REGULAR);
    var_dump ($data);
	echo "Using SORT_NUMERIC:\n";
    krsort ($data, SORT_NUMERIC);
    var_dump ($data);
	echo "Using SORT_STRING\n";
    krsort ($data, SORT_STRING);
    var_dump ($data);
