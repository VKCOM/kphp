@ok
<?php

require_once("Classes/autoload.php");

function test_primitive_types() {
    $less_than_two = function($x) { 
        $x /*:= int*/;
        return $x < 2; 
    };

    $filtered = array_filter([1, 2, 3], $less_than_two);
    $filtered /*:= array<int>*/;
    var_dump($filtered);
}

function test_classes() {
    $less_than_two = function(Classes\IntHolder $c) {
        return $c->a < 2;
    };

    /** @var Classes\IntHolder[] */
    $filtered = array_filter([new Classes\IntHolder(1), new Classes\IntHolder(3)], $less_than_two);
    var_dump(count($filtered));
    var_dump($filtered[0]->a);
}

function test_classes_default_filtered() {
    $filtered = array_filter([new Classes\IntHolder(1), new Classes\IntHolder(3), null]);
    var_dump(count($filtered));
}

test_primitive_types();
test_classes();
test_classes_default_filtered();
