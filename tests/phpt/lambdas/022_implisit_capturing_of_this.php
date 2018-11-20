@todo
<?php

require_once("Classes/autoload.php");

function test_simple_capturing() {
    $a = new Classes\ImplicitCapturingThis(100);
    $a->run();
}

function test_capturing_in_second_level() {
    $a = new Classes\ImplicitCapturingThis(100);
    $a->capturing_in_lambda_inside_lambda();
}

test_simple_capturing();
test_capturing_in_second_level();

