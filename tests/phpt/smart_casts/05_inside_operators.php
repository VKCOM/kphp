@ok
<?php
require_once 'kphp_tester_include.php';

// these tests didn't work while create_full_cfg was used

class BB {
    function f(int $x): string { return (string)$x; }
}

class MyEx extends Exception {
    function __construct(int $x) {}
}


function mInt(int $x): int { return $x*2; }

function getIntOrNull(?int $x): ?int { return $x; }

function acArray(array $x) { return count($x) > 0; }


/** @param int|false $a */
function demo($a) {
    echo ($a + ($a !== false ? mInt($a) : 0)), "\n";
    var_dump(['url' => $a ? mInt($a) . '2' : 'none']);
    echo 'a' . ($a ? mInt($a) : ''), "\n";

    $b = new BB;
    echo "asdf{$b->f($a ? $a : 0)}\n";

    echo 5 < ($a ? mInt($a) : 0) ? "<" : "!<", "\n";
}

function demo2(?int $a) {
    echo getIntOrNull($a) ?? ($a ? mInt($a) : 0), "\n";

    if(0)
        throw new MyEx(is_null($a) ? 0 : mInt($a));

    // todo this still doesn't work
    //echo mInt($a ?: 0), "\n";
}


function demo3($x) {
    if (is_array($x) && acArray($x)) {
        echo "demo3 k ", count($x), "\n";
    }
}



demo(100);
demo(1000);
demo(-1000);
demo(0);
demo(false);

demo2(100);
demo2(null);

demo3(null);
demo3('str');
demo3([1]);
demo3([]);
