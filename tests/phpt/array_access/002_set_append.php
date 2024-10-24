@ok
<?php
require_once 'kphp_tester_include.php';

const ITER_COUNT = 30;
const RAND_STR_LEN = 5;
const MIN_IND = 0;
const MAX_IND = 100500;

const CHAR_SET = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';

/** @return string */
function generate_random_string(int $length) {
    $charactersLength = strlen(CHAR_SET);
    $randomString = '';

    for ($i = 0; $i < $length; $i++) {
        $randomString .= CHAR_SET[rand(0, $charactersLength - 1)];
    }

    return $randomString;
}

function test_sanity() {
    $obj = new Classes\LoggingLikeArray();
    
    $obj[] = "123";
    $obj[] = 42;
    $obj[] = [1, 2, 0.42, "string"];

    var_dump($obj[0]);
    var_dump($obj[1]);
    var_dump($obj[2]);
}

function test_with_seed(int $seed) {
    srand($seed); // different for php and kphp

    $obj = new Classes\LikeArray();
    $arr = [];

    for ($i = 0; $i < ITER_COUNT; $i++) {
        $val1 = generate_random_string(RAND_STR_LEN);
        $val2 = generate_random_string(RAND_STR_LEN);

        /** @var int */
        $index = rand(MIN_IND, MAX_IND);

        $obj[] = $val1;
        $arr[] = $val1;

        $keys = array_keys($arr);
        while (in_array($index, $keys)) {
            $index = rand(MIN_IND, MAX_IND);
        }

        $arr[$index] = $val2;
        $obj[$index] = $val2;
    }



    $keys = array_keys($arr);
    foreach ($keys as $k) {
        assert_true($arr[$k] === $obj[$k]);
    }

}

function test() {
    test_sanity();
    test_with_seed(0);
    test_with_seed(0xdeadbeef);
    test_with_seed(0x42424242);
    test_with_seed(0xfacefeed);
}

test();
