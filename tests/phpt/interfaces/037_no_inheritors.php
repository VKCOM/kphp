@ok
<?php

interface AlwaysAlone {
    function append(int $i): string;
    /**
     * @param tuple(int, string) $arg
     * @return string|false
     */
    function prepend($arg);
    function isOk(): bool;
    function nothing(): void;
    function noop();

    /** @return tuple(int, int[], string[][]|false, AlwaysAlone) */
    function getTuple();
}

function callAlwaysAlone(?AlwaysAlone $arg) {
    if (!$arg) { echo "null\n"; return; }

    $s = $arg->append(3);
    echo $s;
    $arg->nothing();
    $arg->noop();
    $arg->getTuple();

    /** @var string|false $ss */
    $ss = $arg->prepend(tuple(1, 's'));
    var_dump($arg->isOk());
}

callAlwaysAlone(null);

