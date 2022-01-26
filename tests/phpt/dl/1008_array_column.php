@ok
<?php

require_once 'kphp_tester_include.php';

function test_basic() {
    $records = [
        ['id' => 1, 'first_name' => 'John', 'last_name' => 'Doe'],
        ['id' => 2, 'first_name' => 'Sally', 'last_name' => 'Smith'],
        ['id' => 3, 'first_name' => 'Jane', 'last_name' => 'Jones']
    ];
    var_dump(array_column($records, 'first_name'));
    var_dump(array_column($records, 'id'));
    var_dump(array_column($records, 'last_name', 'id'));
    var_dump(array_column($records, 'last_name', 'first_name'));
}

function test_multiple_data_types() {
    $values = [
        ['id' => 2, 'value' => 34.2345],
        ['id' => 3, 'value' => true],
        ['id' => 4, 'value' => false],
        ['id' => 5, 'value' => null],
        ['id' => 6, 'value' => 1234],
        ['id' => 7, 'value' => 'Foo']
    ];
    var_dump(array_column($values, 'value'));
    var_dump(array_column($values, 'value', 'id'));
}

function test_multiple_types_as_index() {
    $a = [
        ["id" => [1]   , "value" => 20], 
        ["id" => "asdf", "value" => 30], 
        ["id" => true  , "value" => 35], 
        ["id" => [2]   , "value" => 25], 
        ["id" => null  , "value" => 40], 
        ["id" => "229" , "value" => 22], 
        ["id" => "229a", "value" => 229],
        ["id" => false , "value" => "new_20_value"]
    ];

    $a = [
        [[1]   , "value" => 20], 
        ["asdf", "value" => 30], 
        [true  , "value" => 35], 
        [[2]   , "value" => 25], 
        [null  , "value" => 40], 
        ["229" , "value" => 22], 
        ["229a", "value" => 229],
        [2.3, "value" => 2.3]
    ];

    var_dump(array_column($a, "value", [0]));
    var_dump(array_column($a, "value", 0));
    var_dump(array_column($a, "value", 0.2));
    var_dump(array_column($a, "value", false));
    var_dump(array_column($a, "value", "id"));
}

function test_numeric_column_keys() {
    $numeric_cols = [
        ['aaa', '111'],
        ['bbb', '222'],
        ['ccc', '333', -1 => 'ddd']
    ];
    var_dump(array_column($numeric_cols, 1));
    var_dump(array_column($numeric_cols, 1, 0));
    var_dump(array_column($numeric_cols, 1, 0.123));
    var_dump(array_column($numeric_cols, 1, -1));

    var_dump(array_column($numeric_cols, 2));
    var_dump(array_column($numeric_cols, 'foo'));
    var_dump(array_column($numeric_cols, 0, 'foo'));
    var_dump(array_column($numeric_cols, 3.14));
}

function test_float_key() {
    $records = [
        ['id' => 1, 'John', 'Doe'],
        ['id' => 2, 'Sally', 'Smith'],
        ['id' => 3, 'Jane', 'Jones']
    ];
    var_dump(array_column($records, 0.2));
    var_dump(array_column($records, 1.4));
    var_dump(array_column($records, 17.25));
    var_dump(array_column($records, 0.001, 1.0023));
    var_dump(array_column($records, 0.001, 'id'));
}

function test_columns_not_present_in_all_rows() {
    $mismatched_columns = [
        ['a' => 'foo', 'b' => 'bar', 'e' => 'bbb'],
        ['a' => 'baz', 'c' => 'qux', 'd' => 'aaa'],
        ['a' => 'eee', 'b' => 'fff', 'e' => 'ggg']
    ];
    var_dump(array_column($mismatched_columns, 'c'));
    var_dump(array_column($mismatched_columns, 'c', 'a'));
    var_dump(array_column($mismatched_columns, 'a', 'd'));
    var_dump(array_column($mismatched_columns, 'a', 'e'));
    var_dump(array_column($mismatched_columns, 'b'));
    var_dump(array_column($mismatched_columns, 'b', 'a'));
}

function test_or_false_value() {
    $or_false_value = [
        ['name' => 1 ? 'Alex' : false],
        ['name' => 0 ? 'Mike' : false]
    ];
    var_dump(array_column($or_false_value, 'name'));
}

function test_or_false_array() {
    $or_false_array = [
        1 ? ['name' => 'Alex'] : false,
        0 ? ['name' => 'Mike'] : false
    ];
    /** @var string[]|false $res */
    $res = array_column($or_false_array, 'name');
    var_dump($res);
}

class A {
    public $x = 0;
    public function __construct($x) {
        $this->x = $x;
    }
}

function test_class_instances() {
    $as = [
        [new A(0), new A(1)],
        [new A(2), new A(3)],
        [new A(4), new A(5)]
    ];
    /** @var A[]|false */
    $odd_a = array_column($as, 1);

    var_dump(array_map(function ($a) { return $a->x; }, $odd_a));

    $optional_classinstance = [true ? ['id' => new A(0)] : false];
    $cls = (array) array_column($optional_classinstance, 'id');
    var_dump(array_map('to_array_debug', $cls));
}

function test_var_as_array() {
    $a = false ? "" : [[1, 2]];
    var_dump(array_column($a, 1));
}

test_basic();
test_multiple_data_types();
test_multiple_types_as_index();
test_float_key();
test_numeric_column_keys();
test_columns_not_present_in_all_rows();
test_or_false_value();
test_or_false_array();
test_class_instances();
test_var_as_array();
