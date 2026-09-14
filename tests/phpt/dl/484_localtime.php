@ok
<?php

function test() {
    $time = 1234567890;//time();

    #var_dump (localtime());
    var_dump (localtime($time));

    #var_dump (localtime(time(), true));
    var_dump (localtime($time, true));
}

date_default_timezone_set ("Etc/GMT-3");
test();
date_default_timezone_set ("Europe/Moscow");
test();
