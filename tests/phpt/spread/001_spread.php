@ok k2_skip
<?php

class Foo {
    public function f() {
        return [1,2,3];
    }
}

/**
 * @return int[]
 */
function arr(): array { return [1,2,4]; }

function print_arr($a) {
    print_r([...$a, 123]);
}

function f2(...$a) {
    return [100, 200, 300];
}

function f() {
    $a = [1, 2, 3];
    var_dump([...$a]);
    var_dump(array(...$a));
    var_dump([1, ...$a]);
    var_dump([1, ...$a, 2]);
    var_dump([1, 2, 3, ...$a, 2]);
    var_dump([1, 2, 3, ...$a, 2, 3, 4]);
    var_dump([...$a, ...$a, 2]);
    var_dump(array(...$a, ...$a, 2));
    var_dump([1, ...$a, ...$a, 2]);
    var_dump([1, ...$a, ...$a, 2, ...$a]);
    var_dump([1, ...$a, 1, 2, 3, ...$a, 2, ...$a]);

    var_dump([...[1, 2, 3]]);
    var_dump([...$a, 100, ...$a, ...$a, ...[1, 2, 3], 100]);
    var_dump(array(...$a, 100, ...$a, ...$a, ...[1, 2, 3], 100));
    var_dump([...$a, 100, ...$a, ...$a, ...([1, 2, 3]), 100]);

    print_arr([...$a]);
    print_arr([...((new Foo)->f())]);

    $f_params = [1];
    var_dump([...$a, ...f2(...$f_params)]);
}

f();
