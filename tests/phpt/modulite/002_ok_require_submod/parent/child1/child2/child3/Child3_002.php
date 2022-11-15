<?php

class Child3_002 {
    static function child3Func() {
        require_once __DIR__ . '/../child3my/Child3My_002.php';
        Child3My_002::child3MyFunc();
        require_once __DIR__ . '/../../child2my/Child2My_002.php';
        return 0;
    }
}
