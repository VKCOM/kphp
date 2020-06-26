@kphp_should_fail
/Incorrect return type of function/
<?php

function run() {
    /**@var $callback callable():void*/
    $callback = function() { return 10; };
    $callback();
}

run();
