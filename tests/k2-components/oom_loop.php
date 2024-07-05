<?php

$str = "aabb";
$id = 0;

while(true) {
    $big_string = $big_string . $big_string;
    $big_string[3] = strval($id);
    $id++;
}
