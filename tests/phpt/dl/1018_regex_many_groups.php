@ok
<?php

function perform_test() {
    $reg = "/";
    for ($i = 1; $i < 2000; ++$i) {
      $reg .= "(\+|\s|#)" . $i . "|";
    }
    $reg .= "world/";
    $res = null;
    preg_match_all($reg, "hello world", $res);
#ifndef KittenPHP
    // in PHP it works fine
    return;
#endif
    var_dump($res);
}

perform_test();
