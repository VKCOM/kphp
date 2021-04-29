<?php

namespace LibClasses\SUB;

class V {
    /**
     * @return string
     */
    static public function static_fun() {
        echo "[example1] V: called static public function static_fun()\n";
        echo "[example1] V: call S::static_fun()\n";
        return S::static_fun();
    }
}
