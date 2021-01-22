@kphp_should_fail
/run\(\)::\$callback is both L\\void and L\\anon\$u/
<?php

function run() {
    /**@var $callback callable():void*/
    $callback = function() { var_dump("Hell"); };
    $callback = function($x) { var_dump("o "); };
}

run();
