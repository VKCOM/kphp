@ok
<?php

/**@param callable(int):string $run*/
function foo($run) {
    var_dump($run(10));
}

function bar() {
    /**@var $run callable(int):string*/
    $run = function($x) { return "here - $x"; };
    foo($run);
}

bar();
