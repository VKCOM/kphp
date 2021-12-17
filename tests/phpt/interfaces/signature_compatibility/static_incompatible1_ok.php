@ok
<?php

// we might expect a message "Incorrect inheritance: count of arguments differ in A::a2 and B::a2"
// but we don't check non-abstract static methods inheritance to make vkcom compile

class A {
    static public function a2() {}
}
class B extends A {
    static public function a2(int $v) {}
}

A::a2();
B::a2(1);


