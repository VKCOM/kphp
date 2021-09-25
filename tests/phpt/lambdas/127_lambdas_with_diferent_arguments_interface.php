@kphp_should_fail
/Count of arguments/
<?php

function run() {
    /**@var $callback callable():void*/
    $callback = function() { var_dump("Hell"); };
    $callback = function($x) { var_dump("o "); };
}

run();
