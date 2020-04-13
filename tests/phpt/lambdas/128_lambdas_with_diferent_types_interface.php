@kphp_should_fail
/Incorrect return type of function: Runnable::__invoke/
<?php

interface Runnable {
    public function __invoke() : void;
}

function run() {
    /**@var $callback callable|Runnable*/
    $callback = function() { return 10; };
    $callback();
}

run();
