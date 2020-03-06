@ok
<?php
require_once 'polyfills.php';
require_once 'Classes/autoload.php';

function getAll() {
    $total_count = 2;
    $description = 'str';
    $items = [
        ['id' => 0, 'name' => 's'],
        ['id' => 1, 'name' => 'r'],
    ];

    return tuple($total_count, $description, $items);
}

function getAllAndA() {
    return shape(['all' => getAll(), 'a' => new Classes\A]);
}

function demo() {
    /** @var Classes\A $a */
    list($total_count, $description, $items) = getAllAndA()['all'];
    $a = getAllAndA()['a'];
    echo $total_count, "\n";
    echo $description, "\n";
    echo count($items), "\n";
    $a->setA(10)->printA();
}

demo();

