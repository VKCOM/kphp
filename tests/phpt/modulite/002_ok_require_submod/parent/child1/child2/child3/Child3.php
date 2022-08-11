<?php

class Child3 {
    static function child3Func() {
        require_once __DIR__ . '/../child3my/Child3My.php';
        Child3My::child3MyFunc();
        require_once __DIR__ . '/../../child2my/Child2My.php';
        return 0;
    }
}
