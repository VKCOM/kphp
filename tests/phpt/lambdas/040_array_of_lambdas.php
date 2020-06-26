@ok
<?php

function run() {
    /**@var (callable():void)[]*/
    $callbacks = [
        function() { var_dump("Hell"); },
        function() { var_dump("o "); },
    ];

    $world = "World!";
    $callbacks[] = function() use ($world) { var_dump($world); };

    foreach ($callbacks as $callback) {
        $callback();
    }
}

run();
