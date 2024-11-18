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

    $obj[100500] = null;
    var_dump(isset($obj[100500]));
    var_dump($obj[100500] == null);
    var_dump($obj[100500] != null);
    var_dump($obj[100500] === null);
    var_dump($obj[100500] !== null);

    $obj[100500] = [1, 2, null];
    unset($obj[100500][1]);
    for ($i = 0; $i < 3; $i++) {
        var_dump(isset($obj[100500][$i]));
        var_dump(!isset($obj[100500][$i]));
        var_dump($obj[100500][$i] == null);
        var_dump($obj[100500][$i] != null);
        var_dump($obj[100500][$i] === null);
        var_dump($obj[100500][$i] !== null);
    }
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

    $obj[100500] = null;
    var_dump(isset($obj[100500]));
    var_dump($obj[100500] == null);
    var_dump($obj[100500] != null);
    var_dump($obj[100500] === null);
    var_dump($obj[100500] !== null);

    $obj[100500] = [1, 2, null];
    unset($obj[100500][1]);
    for ($i = 0; $i < 3; $i++) {
        var_dump(isset($obj[100500][$i]));
        var_dump(!isset($obj[100500][$i]));
        var_dump($obj[100500][$i] == null);
        var_dump($obj[100500][$i] != null);
        var_dump($obj[100500][$i] === null);
        var_dump($obj[100500][$i] !== null);
    }
}

test_obj();
test_mixed_in_obj();
