@kphp_should_fail
/You should override abstract method Printer::print in class MyBool/
<?php

interface Printer {
    function print();
}

enum MyBool implements Printer {
    case T;
    case F;
    public function notPrint() {
        if ($this === self::T) {
            echo "True";
        } else {
            echo "False";
        }
    }
}
