@kphp_should_fail
KPHP_SHOW_ALL_TYPE_ERRORS=1
/return 10;/
/return int from function\(\)/
/but it's declared as @return void/
/ico\(function\(int \$v\) { return \[1.2\]; }\);/
/return float\[\] from function\(\$v\)/
/but it's declared as @return int\[\]/
/\$callback\(3\.4\);/
/pass float to argument \$arg1 of callable\(int\):void/
/but it's declared as @param int/
<?php

function run() {
    /**@var $callback callable():void*/
    $callback = function() {
        return 10;
    };
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
