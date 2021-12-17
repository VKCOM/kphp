@kphp_should_fail
/You may not override field \$i in class B1/
/You may not override field \$j in class D2/
/You may not override field \$k in class C3/
/Cannot make non-static method B4::f4\(\) static/
<?php

class A1 {
    public int $i = 0;
}
class B1 extends A1 {
    public int $i = 1;
}

class A2 {
    public static int $j;
}
class B2 extends A2 {
}
class C2 extends B2 {
}
class D2 extends C2 {
    public int $j;
}

class A3 {
    public int $k;
}
class B3 extends A3 {
}
class C3 extends B3 {
    public static $k;
}

class A4 {
    function f4() {}
}
class B4 extends A4 {
    static function f4() {}
}

