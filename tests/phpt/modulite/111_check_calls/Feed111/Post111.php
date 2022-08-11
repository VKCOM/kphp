<?php

namespace Feed111;

use Utils111\Hidden111;

class Post111 {
    static function demo() {
        \Utils111\Strings111::hidden1();
        Hidden111::demo();
        self::forceHidden();
        plainPublic1();
        plainHidden1();
    }

    static function forceHidden() {
        if(0) Hidden111::demo();
        if(0) globalDemo();
        if(0) \Messages111\Core111\Core111::demo1();
    }

    static function demoRequireUnexported() {
        require_once __DIR__ . '/../parent/child1/child2/child3/Child3.php';
        \Child3::child3Func();
        \Child3::hidden3Func();
    }

    static function demoCallInternalGeneric() {
        \Utils111\Strings111::hiddenGeneric([1,2,3]);
    }
}
