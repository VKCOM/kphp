@ok
<?php
require_once 'kphp_tester_include.php';

class IntWrapper {
    public int $data;
    public function __construct(int $x) {
        $this->data = $x;
    }
}

/** @return mixed */
function print_and_get(int $x) {
    $res = 0;
    for ($i = 0; $i < $x; $i++) {
        $res = (103 * $i + 101 + $res) % 1000000007;
    }
    echo $res . "\n";
    if ($res % 3 == 0) {
        return $res;
    }
    if ($res % 4 == 1) {
        return (string)$res;
    }

    return to_mixed(new IntWrapper($res));
}

function check(int $x) {
    $fut = fork(print_and_get($x));
    echo "Waiting...\n";
    $res = wait($fut);
    if ($res instanceof IntWrapper) {
        echo "IntWrapper!\n";
    }
    if (is_string($res)) {
        echo "string!\n";
    } else if (is_int($res)) {
        echo "int!\n";
    }
}

function main() {
    check(0);
    check(1);
    check(5);
}

main();
