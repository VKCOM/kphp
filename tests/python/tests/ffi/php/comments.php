<?php

function test() {
    $cdef = FFI::cdef('// This is a comments test.

    //
    // Several single-line comments
    // coming one after another.
    //

    // A comment right before the declaration.
    struct Foo {
      int8_t x; // inline single-line comment
    }; // comment //

    /* A multi-line comment. */

    /*
     A multi-line comment
     that contains // token.
    */

    /**
     * phpdoc-style comment.
     */
    /* comment */ struct /*comment*/ Bar /* comment */ {
      int8_t /* mid-statement comment */ y;
    }; /* comment */
    ');

    $foo = $cdef->new('struct Foo');
    $bar = $cdef->new('struct Bar');
    var_dump($foo->x, $bar->y);
}

test();
