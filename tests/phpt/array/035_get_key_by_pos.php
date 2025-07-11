@ok
<?php
require_once 'kphp_tester_include.php';

function get_key_by_pos_in_vector() {
    /** @var int[] */
    $vector = [1, 2, 3];
    var_dump(getKeyByPos($vector, -1));
    var_dump(getKeyByPos($vector, 0));
    var_dump(getKeyByPos($vector, 1));
    var_dump(getKeyByPos($vector, 2));
    var_dump(getKeyByPos($vector, 3));
}

function get_key_by_pos_in_map() {
    $map = ["one" => 1, "two" => 2, "three" => 3];
    var_dump(getKeyByPos($map, -1));
    var_dump(getKeyByPos($map, 0));
    var_dump(getKeyByPos($map, 1));
    var_dump(getKeyByPos($map, 2));
    var_dump(getKeyByPos($map, 3));
}

get_key_by_pos_in_vector();
get_key_by_pos_in_map();
