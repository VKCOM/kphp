@kphp_should_fail
KPHP_SHOW_ALL_TYPE_ERRORS=1
/\$a1 is assumed to be both A and B/
/\$a2 is assumed to be both A and B/
/\$a3 is assumed to be both A and B/
/\$a4 is was declared as @param A, and later reassigned to B/
/\$a5 was already assumed to be A/
/\$a6 has inconsistent phpdocs/
<?php

class A { public int $val = 0; }
class B { public string $s = ''; }

function getB() { return new B; }


function demo1() {
    $a1 = new A;
    $a1->val = 1;
    $a1 = new B;
    $a1->val = 3;
}

function demo2() {
    list($a2) = [new A];
    $a2->val = 1;
    list($a2) = [new B];
    $a2->val = 1;
}

function demo3() {
    /** @var A[] $arr */
    $arr = [new A];
    // auto-assumed to A, that's why not splited
    foreach ($arr as $a3)
        $a3->val = 1;
    if (0)
        $a3 = getB();
    $a3->val = 1;
}

// modification of function arguments with assumptions is prohibited (they are not splitted also)
function demo4(A $a4) {
    $a4 = new B;
    $a4->val;
}

function demo5(A $a5) {
    /** @var B $a5 */
}

function demo6() {
    /** @var A $a6 */
    $a6 = null;
    /** @var B $a6 */
}


demo1();
demo2();
demo3();
demo4(new A);
demo5(new A);
demo6();
