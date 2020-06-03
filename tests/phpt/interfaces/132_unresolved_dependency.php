@kphp_should_fail
/class `A` has unresolved dependencies \[E1, I2, I3, T1]/
<?php

interface I1 {}

class A extends E1 implements I1, I2, I3 {
    use T1;
}
