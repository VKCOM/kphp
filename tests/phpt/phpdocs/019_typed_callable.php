@ok
<?php
// todo пока что phpdoc просто парсится, типизация callable не валидируется

/**
 * @kphp-infer
 * @param callable(int, int) : int $c
 */
function invokator1(callable $c) {
  $result = $c(1, 2);
  echo $result, "\n";
}

invokator1(function($a, $b) { return $a + $b; });
invokator1(function($a, $b) { return $a * $b; });


/**
 * @kphp-infer
 * @param callable(int, int) $c
 */
function invokator2(callable $c) {
  $c(1, 2);
}

invokator2(function($a, $b) { echo $a + $b, "\n"; });
invokator2(function($a, $b) { echo $a * $b, "\n"; });

/**
 * @kphp-infer
 * @param callable() $c
 */
function invokator3(callable $c) {
  $c();
}

invokator3(function() { echo "cb1\n"; });
invokator3(function() { echo "cb2\n"; });

/**
 * @kphp-infer
 * @param callable():int $c
 */
function invokator4(callable $c) {
  $result = $c();
  echo $result, "\n";
}

invokator4(function() { return 1; });
invokator4(function() { return 2; });



/**
 * @return (callable(int):bool)
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
