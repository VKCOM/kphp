@kphp_should_fail
/run\(\)::\$callback is both Runnable and L\\anon\$/
<?php

interface Runnable {
    public function __invoke() : void;
}

function run() {
    /**@var $callback callable|Runnable*/
    $callback = function() { var_dump("Hell"); };
    $callback = function($x) { var_dump("o "); };
}

run();
