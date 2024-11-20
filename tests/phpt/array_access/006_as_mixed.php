@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

/** @param $user mixed */
function dump_user_mixed($user) {
    var_dump($user['id']);
    var_dump($user['name']);
    var_dump($user['friends']);
}

/** @param $user mixed */
function dump_first_friend($user) {
    var_dump($user['friends'][0]);
    var_dump($user['friends']["absent"]);
}

function test() {
    $user_obj = new Classes\User(1, "Paul", [2, 4, 256]);
    $user_arr = ["id" => 1, "name" => "Paul", "friends" => [2, 4, 256]];
    
    dump_user_mixed($user_arr);
    dump_user_mixed(to_mixed($user_obj));

    dump_first_friend(to_mixed($user_obj));
}

test();
