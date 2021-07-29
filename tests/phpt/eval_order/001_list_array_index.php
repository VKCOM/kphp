@ok
<?php

function test() {
    // Using array indexing in list expression is documented as something
    // that can change from one version of PHP to another.
    //
    // PHP7.2 result:          [0 => 777, 1 => 777]
    // PHP7.4 and PHP8 result: [0 => 300, 1 => 777]
    //
    // See: https://bugs.php.net/bug.php?id=71030
    // See: comments in https://www.php.net/list (ctrl+f "undefined")
    //
    // We handle this new case in ConvertListAssignmentsPass.

    $a = [0 => 777, 1 => 300];
    list($a[1], $a[0]) = $a;
    var_dump($a);

    $a2 = [[777, 300], [4, 5, 6], [1, 2, 3]];
    list($a2[0][1], $a2[0][0]) = $a2;
    var_dump($a2);

    $a3 = [[777, 300], [4, 5, 6], [1, 2, 3]];
    list($a3[0][1], $a3[0][0]) = $a3[0];
    var_dump($a3);

    $a4 = [[777, 300], [4, 5, 6], [1, 2, 3]];
    list($a4[1], $a4[0]) = $a4;
    var_dump($a4);
}

test();
