@ok
<?php
require_once 'polyfills.php';

function getAll() {
    $total_count = 2;
    $description = 'str';
    $items = [
        ['id' => 0, 'name' => 's'],
        ['id' => 1, 'name' => 'r'],
    ];

    return tuple($total_count, $description, $items);
}

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

function demo2($returnShort) {
    $result = multipleResult($returnShort);
    echo "total_count = ", $result[0], "\n";
    echo "description = ", $result[1], "\n";
    if ($result[2]) {
        echo "items count = ", count($result[2]), "\n";
    }

    // check that types are correctly inferred
    $result[0] /*:= int */;
    $result[1] /*:= string */;
    $result[2] /*:= OrFalse < array < array < var > > > */;
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
