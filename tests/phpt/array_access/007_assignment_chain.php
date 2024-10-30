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

function test_common_obj_as_mixed() {
    $arr = [1, 2, "asd"];

    $obj = to_mixed(new Classes\LoggingLikeArray([1, 2, 3, "kek" => "lol"]));

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

// TODO $x[.] = $y[.] = $x[] = $y[] = ...

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

test_common();
test_common_as_aa();
test_set_obj_as_mixed();

// TODO
// test_common_obj_as_mixed();
