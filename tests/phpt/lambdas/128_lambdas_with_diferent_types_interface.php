@kphp_should_fail
KPHP_SHOW_ALL_TYPE_ERRORS=1
/return int from anonymous\(...\)/
/but it's declared as @return void/
/return array< float > from anonymous\(...\)/
/but it's declared as @return array< int >/
/pass float to argument \$arg0 of L\\intvoid::__invoke/
/but it's declared as @param int/
<?php

function run() {
    /**@var $callback callable():void*/
    $callback = function() { return 10; };
    $callback();
}

run();


/** @param $callback callable(int):int[] */
function ico($callback) {
    $callback(1);
}

ico(function(int $v) { return [1.2]; });

/** @param $callback callable(int):void */
function ico2($callback) {
    $callback(3.4);
}

ico2(function($asdf) {});
