@ok
<?php
require_once 'kphp_tester_include.php';

class K {
    public int $x;
    public function __construct(int $y) {
        $this->$x = $y;
    }
}

function test_common() {
    $arr = [1, 2, "asd"];

    $obj = new Classes\LoggingLikeArray([1, 2, 3, "kek" => "lol"]);

    $obj[0] = $obj[1] = "str1";
    $obj[4] = $obj[42] = $obj[111] = "";
    $obj[3] = $obj[42] = $arr[4] = $arr["kek"] = $obj[] = $obj["arbidol"] = ["", 0.1, 1, null];

    $keys = $obj->keys();
    foreach ($keys as $key) {
        var_dump($obj[$key]);
    }
}

function test_common_as_aa() {
    $arr = [1, 2, "asd"];

    /** @var \ArrayAccess */
    $obj = new Classes\LoggingLikeArray([1, 2, 3, "kek" => "lol"]);

    $obj[0] = $obj[1] = "str1";
    $obj[4] = $obj[42] = $obj[111] = "";
    $obj[3] = $obj[42] = $arr[4] = $arr["kek"] = $obj[] = $obj["arbidol"] = ["", 0.1, 1, null];

    if ($obj instanceof Classes\LoggingLikeArray) {
        $keys = $obj->keys();
        foreach ($keys as $key) {
            var_dump($obj[$key]);
        }
    } else {
        die_if_failure(false, "Incorrect cast");
    }
}

function test_common_obj_as_mixed() {
    $arr = [1, 2, "asd"];

    $obj = to_mixed(new Classes\LoggingLikeArray([1, 2, 3, "kek" => "lol"]));

    $obj[0] = $obj[1] = "str1";
    $obj[4] = $obj[42] = $obj[111] = "";
    $obj[3] = $obj[42] = $arr[4] = $arr["kek"] = $obj[123456] = $obj["arbidol"] = ["", 0.1, 1, null];
    $obj[3] = $obj[42] = $arr[] = $arr["kek"] = $obj[] = $obj["arbidol"] = ["", 0.1, 1, null];

    if ($obj instanceof Classes\LoggingLikeArray) {
        $keys = $obj->keys();
        foreach ($keys as $key) {
            var_dump($obj[$key]);
        }
    } else {
        die_if_failure(false, "Incorrect cast");
    }
}

function test_set_obj_as_mixed() {
    $obj = new Classes\LoggingLikeArray();
    $obj[3] = $obj[42] = $obj["lol"] = $obj["arbidol"] = to_mixed(new K(42));

    $keys = $obj->keys();
    foreach ($keys as $key) {
        $val = $obj[$key]; // smart casts work only for local variables
        if ($val instanceof K) {
            $val->$x += 1;
            var_dump($val->$x);
        } else {
            die_if_failure(false, "Incorrect cast");
        }
    }
}

/** @param $a mixed */
function dump($a) {
    if ($a instanceof \Classes\KeysableArrayAccess) {
        $keys = $a->keys();
        foreach ($keys as $key) {
            var_dump($a[$key]);
        }
    } else {
        die_if_failure(false, "Incorrect smart cast");
    }
}

function test_multiple_objects() {
    $obj1 = to_mixed(new Classes\LoggingLikeArray([1, 2, 3, "kek" => "lol"]));
    $obj2 = to_mixed(new Classes\LoggingLikeArray([]));
    $obj3 = to_mixed(new Classes\LikeArray([500, 501, 502, 503]));
    $obj4 = new Classes\LoggingLikeArray(["str", 0.541, "key" => "val"]);

    $obj1[] = $obj2[] = $obj1[] = $obj3[] = $obj4[] = "first";
    $obj1[1] = $obj2[] = $obj1[1] = $obj3[23] = $obj4["str_key"] = "second";
    $obj3[100] = $obj2[100] = $obj3[] = $obj2[] = $obj4[0];
    $obj4[] = $obj4[] = $obj4[1] = $obj1[] = $obj1[] = $obj1[] = $obj4["key"];

    dump($obj1);
    dump($obj2);
    dump($obj3);
    dump(to_mixed($obj4));
}

test_common();
test_common_as_aa();
test_common_obj_as_mixed();
test_set_obj_as_mixed();
test_multiple_objects();
