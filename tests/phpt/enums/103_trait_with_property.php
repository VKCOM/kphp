@kphp_should_fail
<?php

trait Logger {
    public static int $x = 42;
    public function Print() { echo self::$x; }
}

enum MyBool {
    use Logger;
    case T;
    case F;
}

MyBool::T->Print();
