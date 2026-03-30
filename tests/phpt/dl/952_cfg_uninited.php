@kphp_should_warn
/Variable \$a1 may be used uninitialized/
/Variable \$a2 may be used uninitialized/
/Variable \$b2 may be used uninitialized/
/Variable \$a3 may be used uninitialized/
/Variable \$a4 may be used uninitialized/
<?php


function demo1() {
    if (1)
        $a1 = 1;
    echo $a1;
}

function demo2() {
    if (1)
        $a2 = 1;
    else
        $b2 = '1';
    echo $a2;
    echo $b2;
}

function demo3() {
    for ($i = 0; $i < 10; ++$i) {
        $a3 = $i;
    }
    echo $a3;
}

function demo4() {
    if (rand()) {
        $a4 = 1;
    } else if(rand()) {
        $a4 = 2;
    } else if(rand()) {
        list(, $a4) = [1,3];
    }

    if(rand()) {
        $result = $a4 * 2;
    }
}


demo1();
demo2();
demo3();
demo4();

