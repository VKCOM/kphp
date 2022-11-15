<?php

namespace Utils111;

class Strings111 {
    static function normal() {
        Hidden111::demo();
        new \GlobalCl111;
        \GlobalCl111::staticFn();
    }

    static function hidden1() { }
    static function hidden2() { }

    /**
     * @kphp-generic T
     * @param T[] $arr
     */
    static function hiddenGeneric($arr) {
        var_dump($arr);
    }
}
