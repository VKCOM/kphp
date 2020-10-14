<?php

namespace Classes;

class UseCallbacksAsString
{

    var $a = 10;
    public static function use_callback_static(callable $callback) {
        $callback(100);
    }

    /**
     * @kphp-required
     * @param int $x
     */
    public static function f_print_static($x) {
        var_dump($x);
    }

    /**
     * @return null
     */
    public static function run_static() {
        self::use_callback_static(['\Classes\UseCallbacksAsString', 'f_print_static']);
        return null;
    }

    /**
     * @param int $x
     * @return int
     */
    public static function call_me_static($x) {
        return $x + 1;
    }

    public static function run_me_static() {
        var_dump(UseCallbacksAsString::call_me_static(10));
        $a = array_map(['Classes\UseCallbacksAsString', 'call_me_static'], [1, 2, 3]);
        var_dump($a);
    }

    /**
     * @kphp-required
     * @return int
     */
    public function get_aaa() {
        return $this->a;
    }

    /**
     * @kphp-required
     * @return int
     */
    public function get_bbb() {
        return 1000 + $this->a;
    }

    public function use_callback($a, callable $callback) {
        return $callback();
    }
}
