<?php

namespace Messages003;

class User003 {
    static function demo() {
        // "allow-internal" is written for "\\Messages003\\User003::demo()"
        // real usage in inside a lambda, but the rule works correctly
        (function() {
        (function() {
        if(0) \Feed003\Post003::demo();
        if(0) \Utils003\Strings003::normal();
        if(0) \Utils003\Hidden003::demo2();
        if(0) Core003\Core003::demo1();
        echo \Feed003\Post003::ONE, ' ', \Feed003\Post003::HIDDEN, ' ', PLAIN_CONST_PUB_003, "\n";
        echo Core003\Core003::SMALL, "\n";
        echo \Feed003\Post003::$ONE, ' ', \Feed003\Post003::$HIDDEN, "\n";
        echo Core003\Core003::$SMALL, "\n";
        /** @var $i ?Core003\Core003 */
        $i = null;
        })();
        })();
    }
}
