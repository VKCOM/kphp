@kphp_should_fail
/lambda can\'t implement interface: L\\void/
<?php

function run() {
    /**@var $callback callable():void*/
    $callback = function() { var_dump("Hell"); };
    $callback = function($x) { var_dump("o "); };
}

run();
