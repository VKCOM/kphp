@ok
<?php

function g(): int { return 1; }

/**
 * @return int[]
 */
function arr(): array { return [1,2,4]; }

function f() {
    $a = [1,2,3];
    var_dump([...$a]);
    var_dump([1, ...$a]);
    var_dump([1, ...$a, 2]);
    var_dump([1, 2, 3, ...$a, 2]);
    var_dump([1, 2, 3, ...$a, 2, 3, 4]);
    var_dump([...$a, ...$a, 2]);
    var_dump([1, ...$a, ...$a, 2]);
    var_dump([1, ...$a, ...$a, 2, ...$a]);
    var_dump([1, ...$a, 1, 2, 3, ...$a, 2, ...$a]);

    var_dump([...[1,2,3]]);
    var_dump([...$a, 100, ...$a, ...$a, ...[1,2,3], 100]);
}

f();

