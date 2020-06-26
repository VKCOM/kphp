@ok
<?php
function run() {
    $world = "World!";
    /**@var (callable():void)[]*/
    $callbacks = [];

    /**@var $callback callable():void*/
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
