@ok
<?php

require_once 'kphp_tester_include.php';

function call_interface(Classes\IDo $can_do) {
    if ($can_do instanceof Classes\A) {
        $a = instance_cast($can_do, Classes\A::class);
        $a->do_it(10, 20);
    } else if ($can_do instanceof Classes\B) {
        $b = instance_cast($can_do, Classes\B::class);
        $b->do_it(10000, 20000);
    }
}

call_interface(new Classes\A());
call_interface(new Classes\B());

