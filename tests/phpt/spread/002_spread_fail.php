@kphp_should_fail
/Only arrays can be unpacked with spread \(\.\.\.\$a\) operator\, but passed type 'int'/
/Only arrays can be unpacked with spread \(\.\.\.\$a\) operator\, but passed type 'int'/
<?php

function g(): int { return 1; }

function f() {
    $a = [1,2,3];
    var_dump([...g()]);
    var_dump([...$a, 1, 2, 3, ...g()]);
}

f();
