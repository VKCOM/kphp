@ok k2_skip
<?php

// earlier, modification of used vars inside lambdas was prohibited
// (because use($x) { $x++; } was compiled into { v$this->$x++; })
// now, it's allowed and works as in PHP
// (becase it's now compiled to { v$x = v$this->$x; v$x++; })

function inc(&$x) {
    $x += 1;
}

function demo1() {
    $i = 10;
    $f = function($x) use ($i) { inc($i); return $x + $i; };
    $mapped = array_map($f, [1, 2, 3]);
    var_dump($mapped);
    var_dump($i);
}

function demo2() {
    $i = [10];
    $f = function($x) use ($i) {
        foreach($i as &$it) { $it += 1; }
        return $i[0];
    };
    var_dump(array_map($f, [1, 2, 3]));
}

function demo3() {
    $i = [1];
    $f = function($x) use ($i) { $i[0] += 1; return $x + $i[0]; };
    $mapped = array_map($f, [1, 2, 3]);
    var_dump($mapped);
    var_dump($i);
}

function demo4() {
    $x = 'x';
    $f1 = fn() => print_r($x = 'y');
    $f2 = fn() => print_r(list($x) = ['y2']);
    $f1();
    $f2();
    var_dump($x);
}

demo1();
demo2();
demo3();
demo4();
