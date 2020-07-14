<?php

namespace Classes;

class A {
    /**
     * @kphp-infer
     * @return string
     */
    static public function some_function() {
        echo "[lib_user] A: called some_function()\n";
        return "some function call";
    }
}
