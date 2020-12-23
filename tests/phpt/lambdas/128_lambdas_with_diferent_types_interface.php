@kphp_should_fail
/return int from anonymous\(...\)/
<?php

function run() {
    /**@var $callback callable():void*/
    $callback = function() { return 10; };
    $callback();
}

run();
