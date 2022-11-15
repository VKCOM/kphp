<?php

class Child3_106 {
    static function child3Func() {
        require_once __DIR__ . '/../child3my/Child3My_106.php';
        Child3My_106::child3MyFunc();
        require_once __DIR__ . '/../../child2my/Child2My_106.php';
        Child2My_106::child2MyFunc();
        return 0;
    }
}
