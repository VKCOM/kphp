@ok php8
<?php

include "interfaces/Printer.php";

enum MyBool implements interfaces\PrinterNS\Printer {
    case T;
    case F;
    public function print() {
        if ($this === self::T) {
            echo "True";
        } else {
            echo "False";
        }
    }
}

MyBool::T->print();
MyBool::F->print();
