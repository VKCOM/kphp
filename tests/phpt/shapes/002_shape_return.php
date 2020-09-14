@ok
<?php
require_once 'kphp_tester_include.php';

/**
 * @kphp-infer
 * @return shape(items:mixed[][], desc:string, count:int)
 */
function getAll() {
    $total_count = 2;
    $description = 'str';
    $items = [
        ['id' => 0, 'name' => 's'],
        ['id' => 1, 'name' => 'r'],
    ];

    return shape(['count' => $total_count, 'desc' => $description, 'items' => $items]);
}

/**
 * @kphp-infer
 * @param bool $returnShort
 * @return shape(items:mixed[][]|false, desc:string, count:int)
 */
function multipleResult($returnShort) {
    $total_count = 2;
    $description = 'str';

    if ($returnShort) {
        return shape(['count' => $total_count, 'desc' => $description, 'items' => false]);
    }

    $items = [
        ['id' => 0, 'name' => 's'],
        ['id' => 1, 'name' => 'r'],
    ];

    return shape(['count' => $total_count, 'desc' => $description, 'items' => $items]);
}

/**
 * @kphp-infer
 * @return shape(3:mixed, o:mixed, t:mixed)
 */
function badMixResult() {   // works, but infers shape<var,var,var>
    return 1
        ? shape(['o' => 1, 't' => 'string', '3' => [1,2,3]])
        : shape(['o' => '1', 't' => 1, '3' => 'string']);
}

function demo() {
    $result = getAll();
    echo "total_count = ", $result['count'], "\n";
    echo "description = ", $result['desc'], "\n";
    echo "items count = ", count($result['items']), "\n";
    echo "items[1] id = ", $result['items'][1]['id'], "\n";
}

/**
 * @kphp-infer
 * @param bool $returnShort
 */
function demo2($returnShort) {
    $result = multipleResult($returnShort);
    echo "total_count = ", $result['count'], "\n";
    echo "description = ", $result['desc'], "\n";
    if ($result['items']) {
        echo "items count = ", count($result['items']), "\n";
    }

    // check that types are correctly inferred
    /** @var int $item_0 */
    /** @var string $item_1 */
    /** @var mixed[][]|false $item_2 */
    $item_0 = $result['count'];
    $item_1 = $result['desc'];
    $item_2 = $result['items'];
}

function demo3() {
    $result = badMixResult();
    $var1 = $result['o'];
    $var2 = $result['t'];
    $var3 = $result['3'];
    echo $var1, ' ', $var2, "\n";
}

demo();
demo2(false);
demo2(true);
demo3();
