@kphp_should_fail
/insert int\[\] into \$y\[\]/
/but it's declared as @var int/
<?php

function main() {
    /** @var int[] */
    $y = [];
    
    /** @var int[] */
    $x = [];
    $y[0] = $x;
}

main();
