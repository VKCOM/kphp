<?php

namespace Feed111;

use Utils111\Hidden111;

class Post111 {
    static function demo() {
        \Utils111\Strings111::hidden1();
        Hidden111::demo();
        self::forceHidden();
        plainPublic1_111();
        plainHidden1_111();
    }

    static function forceHidden() {
        if(0) Hidden111::demo();
        if(0) globalDemo();
        if(0) \Messages111\Core111\Core111::demo1();
    }

    static function demoRequireUnexported() {
        require_once __DIR__ . '/../parent/child1/child2/child3/Child3_111.php';
        \Child3_111::child3Func();
        \Child3_111::hidden3Func();
    }

    static function demoCallInternalGeneric() {
        \Utils111\Strings111::hiddenGeneric([1,2,3]);
    }
}
