@ok
<?php
require_once 'kphp_tester_include.php';

function get_value_by_pos_in_vector() {
    /** @var int[] */
    $vector = [1, 2, 3];
    var_dump(getValueByPos($vector, -1));
    var_dump(getValueByPos($vector, 0));
    var_dump(getValueByPos($vector, 1));
    var_dump(getValueByPos($vector, 2));
    var_dump(getValueByPos($vector, 3) == 0); // Polyfill returns NULL, kphp returns 0
}

function get_value_by_pos_in_map() {
    $map = ["one" => 1, "two" => 2, "three" => 3];
    var_dump(getValueByPos($map, -1));
    var_dump(getValueByPos($map, 0));
    var_dump(getValueByPos($map, 1));
    var_dump(getValueByPos($map, 2));
    var_dump(getValueByPos($map, 3) == 0); // Polyfill returns NULL, kphp returns 0
}

get_value_by_pos_in_vector();
get_value_by_pos_in_map();
