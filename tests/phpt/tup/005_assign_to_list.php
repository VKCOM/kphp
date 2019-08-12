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
    $total_count = 2;
    $description = 'str';
    $items = [
        ['id' => 0, 'name' => 's'],
        ['id' => 1, 'name' => 'r'],
    ];

    return tuple($total_count, $description, $items, new Classes\A);
}

function demo1() {
    list($int, $string) = tuple(1, 'str');
    echo $int, ' ', $string, "\n";
}

function demo2() {
    /** @var $a Classes\A */
    list($int, $a) = tuple(1, new Classes\A);
    echo $int, ' ', $a->a, "\n";
    $a->setA(9)->printA();
}

function demo3() {
    list($total_count, $description, $items) = getAll();
    echo $total_count, "\n";
    echo $description, "\n";
    echo count($items), "\n";
}

function demo4() {
    /** @var Classes\A $a */
    list($total_count, $description, $items, $a) = getAllAndA();
    echo $total_count, "\n";
    echo $description, "\n";
    echo count($items), "\n";
    $a->setA(10)->printA();
}

demo1();
demo2();
demo3();
demo4();

