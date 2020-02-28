@ok
<?php
require_once 'polyfills.php';

function getNextArray() {
    static $idx = 0;
    $arrays = [
        [1, 's1'],
        [2, 's2'],
        [3, 's3'],
    ];
    if ($idx >= count($arrays)) {
        return false;
    }
    return $arrays[$idx++];
}

function getNextTuple() {
    static $idx = 0;
    $tuples = [
        tuple(1, 's1'),
        tuple(2, 's2'),
        tuple(3, 's3'),
    ];
    if ($idx >= count($tuples)) {
        return false;
    }
    return $tuples[$idx++];
}

function demo() {
    /**
     * @var mixed $i
     * @var mixed $s
     */
    while (list($i, $s) = getNextArray()) {
        echo $i, ' ', $s, "\n";
    }

    /**
     * @var int $i
     * @var string $s
     */
    while (list($i, $s) = getNextTuple()) {
        echo $i, ' ', $s, "\n";
    }
}

demo();
