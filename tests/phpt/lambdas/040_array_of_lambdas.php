@ok
<?php
interface Runnable {
    public function __invoke() : void;
}

function run() {
    /**@var (callable|Runnable)[]*/
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
