@ok
<?php
require_once 'kphp_tester_include.php';

// TODO
// Check [.][.] and longer chains, too

/** @param $user mixed */
function dump_user_mixed($user) {
    var_dump($user['id']);
    var_dump($user['name']);
    var_dump($user['friends']);
}

function test() {
    $user_obj = new Classes\User(1, "Paul", [2, 4, 256]);
    $user_arr = ["id" => 1, "name" => "Paul", "friends" => [2, 4, 256]];
    dump_user_mixed($user_arr);
    dump_user_mixed(to_mixed($user_obj));
}

test();
