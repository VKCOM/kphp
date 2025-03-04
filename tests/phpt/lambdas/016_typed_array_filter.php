@ok
<?php

require_once 'kphp_tester_include.php';

function test_primitive_types() {
    $less_than_two = function(int $x) {
        return $x < 2;
    };

    /** @var int[] */
    $filtered = array_filter([1, 2, 3], $less_than_two);
    var_dump($filtered);
}

function test_classes() {
    $less_than_two = function(Classes\IntHolder $c) {
        return $c->a < 2;
    };

    /** @var Classes\IntHolder[] */
    $filtered = array_filter([new Classes\IntHolder(1), new Classes\IntHolder(3)], $less_than_two);
    var_dump(count($filtered));
    var_dump($filtered[0]->a);
}

function test_classes_default_filtered() {
    $filtered = array_filter([new Classes\IntHolder(1), new Classes\IntHolder(3), null]);
    var_dump(count($filtered));
}

function test_primitive_types_filter_by_key() {
    $is_odd = function($key) {
        return $key % 2 != 0;
    };

    /** @var string[] */
    $filtered = array_filter_by_key(["x", "y", "z", 99 => "u", 100 => "v"], $is_odd);
    var_dump($filtered);
}

function test_classes_filter_by_key() {
    $is_short = function($key) {
        return strlen($key) < 6;
    };

    /** @var Classes\IntHolder[] */
    $filtered = array_filter_by_key([
      "short" => new Classes\IntHolder(1),
      "x" => new Classes\IntHolder(2),
      "very long key" => new Classes\IntHolder(3),
      9 => new Classes\IntHolder(4),
      9123124 => new Classes\IntHolder(5)
    ], $is_short);

    foreach ($filtered as $key => $value) {
      var_dump($key);
      var_dump(to_array_debug($value));
    }
}

test_primitive_types();
test_classes();
test_classes_default_filtered();
test_primitive_types_filter_by_key();
test_classes_filter_by_key();
