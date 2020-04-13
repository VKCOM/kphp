@ok
<?php
interface CorrectId {
    public function __invoke(int $x) : bool;
}

/**
 * @return CorrectId|callable
 */
function get_checker(bool $get_even) {
    if ($get_even) {
        return function (int $x) : bool { return $x % 2 == 0; };
    } else {
        return function (int $x) : bool { return $x % 2 != 0; };
    }
}

function run() {
    $checker = get_checker(true);
    var_dump($checker(1));

    $checker = get_checker(false);
    var_dump($checker(1));
}

run();
