<?php

class ParentFuncs {
    static function testThatCantAccessSubsubchild() {
        require_once __DIR__ . '/child1/child2/child3/Child3.php';
        Child3::child3Func();
    }
}
