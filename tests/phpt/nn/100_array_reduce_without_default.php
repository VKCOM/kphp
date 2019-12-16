@kphp_should_fail
<?php

$ints = [[1, 2], [3, 4]];
$new_list = array_reduce($ints, function($items, $item) {
    $items[] = $item[0];
    $items[] = $item[1];
    return $items;
});
