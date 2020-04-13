@ok
<?php
interface Runnable {
    public function __invoke() : void;
}

function run() {
    $world = "World!";
    /**@var (callable|Runnable)[]*/
    $callbacks = [];

    /**@var $callback callable|Runnable*/
    $callback = function() { var_dump("Hell"); };
    $callbacks[] = $callback;

    $callback = function() { var_dump("o "); };
    $callbacks[] = $callback;

    $callback = function() use ($world) { var_dump($world); };
    $callbacks[] = $callback;
     
    foreach ($callbacks as $callback) {
        $callback();
    }
}

run();
