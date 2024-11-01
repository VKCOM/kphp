@ok
<?php
require_once 'kphp_tester_include.php';

function test_obj() {
    $obj = new Classes\LoggingLikeArray(["zero", "one", "two", "str_key" => 42]);

    var_dump(isset($obj[0]));
    var_dump(isset($obj["str_key"]));
    var_dump(isset($obj["non-existing-key"]));

    unset($obj["str_key"]);
    unset($obj[0]);

    $keys = $obj->keys();
    foreach ($keys as $key) {
        var_dump($obj[$key]);
    }

    $obj[0] = "new";
    var_dump(isset($obj[0]));
}

function test_mixed_in_obj() {
    $obj = to_mixed(new Classes\LoggingLikeArray(["zero", "one", "two", "str_key" => 42]));

    var_dump(isset($obj[0]));
    var_dump(isset($obj["str_key"]));
    var_dump(isset($obj["non-existing-key"]));

    unset($obj["str_key"]);
    unset($obj[0]);

    if ($obj instanceof Classes\KeysableArrayAccess) {
        $keys = $obj->keys();
        foreach ($keys as $key) {
            var_dump($obj[$key]);
        }
    }

    $obj[0] = "new";
    var_dump(isset($obj[0]));
}

test_obj();
test_mixed_in_obj();
