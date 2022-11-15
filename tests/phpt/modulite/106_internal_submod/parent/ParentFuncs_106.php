<?php

class ParentFuncs_106 {
    static function testThatCantAccessSubsubchild() {
        require_once __DIR__ . '/child1/child2/child3/Child3_106.php';
        require_once __DIR__ . '/morechild1/MoreChild1.php';
        Child3_106::child3Func();
    }
}
