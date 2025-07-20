@ok
<?php

function basic_test() {
    $array1 = array("a" => "green", "b" => "brown", "c" => "blue", "red", "");
    $array2 = array("a" => "green", "yellow", "red", TRUE);
    $array3 = array("red", "a" => "brown", "");
    var_dump(array_intersect_assoc($array1, $array2));
    var_dump(array_intersect_assoc($array1, $array3));
    var_dump(array_intersect_assoc($array2, $array3));
    var_dump(array_intersect_assoc($array1, $array2, $array3));
}

function different_keys_test() {
    $array_default_key = array('one', 2, 'three', '4');
    $array_numeric_key = array(1 => 'one', 2 => 'two', 3 => 4);
    $array_string_key  = array('one' => 1, 'two' => '2', '3' => 'three');

    var_dump(array_intersect_assoc($array_default_key, $array_numeric_key));
    var_dump(array_intersect_assoc($array_numeric_key, $array_default_key));

    var_dump(array_intersect_assoc($array_default_key, $array_numeric_key));
    var_dump(array_intersect_assoc($array_numeric_key, $array_default_key));

    var_dump(array_intersect_assoc($array_numeric_key, $array_string_key));
    var_dump(array_intersect_assoc($array_string_key, $array_numeric_key));
}

function variation_array_test() {
    $array = [1, 2, 3];
    //get an unset variable
    $unset_var = 10;
    unset ($unset_var);

    var_dump(array_intersect_assoc([0, 1, 12345, -2345], $array));
    var_dump(array_intersect_assoc([10.5, -10.5, 12.3456789000e10, 12.3456789000E-10, .5], $array));
    var_dump(array_intersect_assoc([NULL, null], $array));
    var_dump(array_intersect_assoc([true, false, TRUE, FALSE], $array));
    var_dump(array_intersect_assoc(["", ''], $array));
    var_dump(array_intersect_assoc(["string"], $array));
    var_dump(array_intersect_assoc([@$unset_var], $array));
}

function variation_map_keys_type1_test() {
    $array = [1, 2, 3];
    //get an unset variable
    $unset_var = 10;
    unset ($unset_var);

    var_dump(array_intersect_assoc([
        0 => 'zero', 1 => 'one', 12345 => 'positive', -2345 => 'negative'
    ], $array));

    var_dump(array_intersect_assoc([
        10.5 => 'float 1', -10.5 => 'float 2', .5 => 'float 3'
    ], $array));

    var_dump(array_intersect_assoc([
        true => 'boolt', false => 'boolf', TRUE => 'boolT', FALSE => 'boolF'
    ], $array));

    var_dump(array_intersect_assoc([NULL => 'null 1', null => 'null 2'], $array));
    var_dump(array_intersect_assoc(["" => 'emptyd', '' => 'emptys'], $array));
    var_dump(array_intersect_assoc(["string" => 'stringd', 'string' => 'strings'], $array));
    var_dump(array_intersect_assoc([@$unset_var => 'unset'], $array));
}

function variation_map_keys_type2_test() {
    $array = ['zero', 1 => 1, 'two' => 2.00000000000001];

    var_dump(array_intersect_assoc(['2.00000000000001', '1', 'zero', 'a'], $array));
    var_dump(array_intersect_assoc([2 => '2.00000000000001', 1 => '1', 0 => 'zero', 3 => 'a'], $array));
    var_dump(array_intersect_assoc(['2' => '2.00000000000001', '1' => '1', '0' => 'zero', '3' => 'a'], $array));
    var_dump(array_intersect_assoc([2, 1, 0], $array));
    var_dump(array_intersect_assoc([2 => 2, 1 => 1, 0 => 0], $array));
    var_dump(array_intersect_assoc(['two' => 2, '1' => 1, '0' => 0], $array));
    var_dump(array_intersect_assoc([2.00000000000001, 1.00, 0.01E-9], $array));
    var_dump(array_intersect_assoc([2 => 2.00000000000001, 1 =>  1.00, 0 => 0.01E-9], $array));
    var_dump(array_intersect_assoc(['two' => 2.00000000000001, '1' => 1.00, '0' =>0.01E-9], $array));
}

function variation_usage_test() {
    $arr_default_int = [1, 2, 3, 'a'];
    $arr_float = [0 => 1.00, 1.00 => 2.00, 2.00 => 3.00, 'b'];
    $arr_string = ['1', '2', '3', 'c'];
    $arr_string_float = ['0' => '1.00', '1.00' => '2.00', '2.00' => '3.00', 'd'];

    var_dump(array_intersect_assoc($arr_default_int, $arr_float));
    var_dump(array_intersect_assoc($arr_float, $arr_default_int));

    var_dump(array_intersect_assoc($arr_default_int, $arr_string));
    var_dump(array_intersect_assoc($arr_string, $arr_default_int));

    var_dump(array_intersect_assoc($arr_default_int, $arr_string_float));
    var_dump(array_intersect_assoc($arr_string_float, $arr_default_int));

    var_dump(array_intersect_assoc($arr_float, $arr_string));
    var_dump(array_intersect_assoc($arr_string, $arr_float));

    var_dump(array_intersect_assoc($arr_float, $arr_string_float));
    var_dump(array_intersect_assoc($arr_string_float, $arr_float));

    var_dump(array_intersect_assoc($arr_string, $arr_string_float));
    var_dump(array_intersect_assoc($arr_string_float, $arr_string));

    var_dump(array_intersect_assoc($arr_default_int, $arr_float, $arr_string));
    var_dump(array_intersect_assoc($arr_default_int, $arr_float, $arr_string_float));
    var_dump(array_intersect_assoc($arr_default_int, $arr_string, $arr_string_float));
    var_dump(array_intersect_assoc($arr_float, $arr_string, $arr_string_float));
}

function key_value_swaps_test() {
    $array_index = ['a', 'b', 'c', 0 => 'd', 'b'];
    $array_assoc = [
        '2' => 'c', //same key=>value pair, different order
        '1' => 'b',
        '0' => 'a',
        'b' => '3', //key and value from array_index swapped
        'c' => 2 //same as above, using integer
    ];

    var_dump(array_intersect_assoc($array_index, $array_assoc));
    var_dump(array_intersect_assoc($array_assoc, $array_index));
}

function sub_arrays_test() {
    $array1 = ['sub_array1' => [1, 2, 3], 'sub_array2' => ['a', 'b', 'c']];
    $array2 = ['sub_arraya' => [1, 3, 5], 'sub_arrayb' => ['a', 'z', 'y']];

    var_dump(array_intersect_assoc($array1, $array2));
    var_dump(array_intersect_assoc($array2, $array1));

    var_dump(array_intersect_assoc($array1['sub_array1'], $array2['sub_arraya']));
    var_dump(array_intersect_assoc($array2['sub_arraya'], $array1['sub_array1']));
    var_dump(array_intersect_assoc($array1['sub_array2'], $array2['sub_arrayb']));
    var_dump(array_intersect_assoc($array2['sub_arrayb'], $array1['sub_array1']));

    var_dump(array_intersect_assoc($array1['sub_array1'], $array2));
    var_dump(array_intersect_assoc($array1, $array2['sub_arraya']));
}

basic_test();
different_keys_test();
variation_array_test();
variation_map_keys_type1_test();
variation_map_keys_type2_test();
variation_usage_test();
key_value_swaps_test();
sub_arrays_test();
