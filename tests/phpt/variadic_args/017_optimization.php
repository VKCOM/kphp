@ok
<?php

function g(int $lvl, ...$args) {
    var_dump([$args]);
}

function foo(...$args) {
    g(1, ...[...$args, ...$args]);
}

foo(1, 2, 3);
