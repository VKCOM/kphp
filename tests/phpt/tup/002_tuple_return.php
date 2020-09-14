@ok
<?php
require_once 'kphp_tester_include.php';

/**
 * @kphp-infer
 * @return tuple(int, string, mixed[][])
 */
function getAll() {
    $total_count = 2;
    $description = 'str';
    $items = [
        ['id' => 0, 'name' => 's'],
        ['id' => 1, 'name' => 'r'],
    ];

    return tuple($total_count, $description, $items);
}

/**
 * @kphp-infer
 * @param bool $returnShort
 * @return tuple(int, string, mixed[][]|false)
 */
function multipleResult($returnShort) {
    $total_count = 2;
    $description = 'str';

    if ($returnShort) {
        return tuple($total_count, $description, false);
    }

    $items = [
        ['id' => 0, 'name' => 's'],
        ['id' => 1, 'name' => 'r'],
    ];

    return tuple($total_count, $description, $items);
}

/**
 * @kphp-infer
 * @return tuple(mixed, mixed, mixed)
 */
function badMixResult() {   // works, but infers tuple<var,var,var>
    return 1
        ? tuple(1, 'string', [1,2,3])
        : tuple('1', 1, 'string');
}

function demo() {
    $result = getAll();
    echo "total_count = ", $result[0], "\n";
    echo "description = ", $result[1], "\n";
    echo "items count = ", count($result[2]), "\n";
    echo "items[1] id = ", $result[2][1]['id'], "\n";
}

/**
 * @kphp-infer
 * @param bool $returnShort
 */
function demo2($returnShort) {
    $result = multipleResult($returnShort);
    echo "total_count = ", $result[0], "\n";
    echo "description = ", $result[1], "\n";
    if ($result[2]) {
        echo "items count = ", count($result[2]), "\n";
    }

    // check that types are correctly inferred
    /** @var int $item_0 */
    /** @var string $item_1 */
    /** @var mixed[][]|false $item_2 */
    $item_0 = $result[0];
    $item_1 = $result[1];
    $item_2 = $result[2];
}

function demo3() {
    $result = badMixResult();
    $var1 = $result[0];
    $var2 = $result[1];
    $var3 = $result[2];
    echo $var1, ' ', $var2, "\n";
}

demo();
demo2(false);
demo2(true);
demo3();
