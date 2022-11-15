<?php

class ParentFuncs_002 {
    static function testThatCantAccessSubsubchild() {
        require_once __DIR__ . '/child1/child2/child3/Child3_002.php';
        Child3_002::child3Func();
    }
}
