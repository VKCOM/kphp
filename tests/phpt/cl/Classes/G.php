<?php

namespace Classes;

class G {
    static private $static_private = "static private";
    private static $private_static = "private static";

    static protected $static_protected = "static protected";
    protected static $protected_static = "protected static";

    static public $static_public = "static public";
    public static $public_static = "public static";
    static $just_static          = "static";

    static private function static_private_func() {
        echo G::$static_private, "\n";
        echo G::$private_static, "\n";

        echo G::$static_protected, "\n";
        echo G::$protected_static, "\n";

        echo G::$static_public, "\n";
        echo G::$public_static, "\n";
    }

    private static function private_static_func() {
        G::static_private_func();
    }

    static protected function static_protected_func() {
        G::private_static_func();
    }

    protected static function protected_static_func() {
        G::static_protected_func();
    }

    static public function static_public_func() {
        G::protected_static_func();
    }

    public static function public_static_func() {
        G::static_public_func();
    }

    static function static_func() {
        G::static_public_func();
    }
}
