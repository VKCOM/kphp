@ok
<?php

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

function test_single_dimensional_array() {
    $single_dimension = ['foo', 'bar', 'baz'];
    var_dump(array_column($single_dimension, 1));
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
    var_dump(array_column($or_false_array, 'name'));
}


test_basic();
test_multiple_data_types();
test_float_key();
test_numeric_column_keys();
test_single_dimensional_array();
test_columns_not_present_in_all_rows();
test_or_false_value();
test_or_false_array();
