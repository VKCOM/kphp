@kphp_should_fail
KPHP_ENABLE_FFI=1
/Field \$value not found in class cdata\$cdef\$u(\w+)_0\\Bar/
<?php

/**
 * @kphp-template T
 * @param T $struct
 */
function printXY($struct) {
    echo $struct->value, "\n";
}

$cdef = FFI::cdef('
    struct Foo {
      int8_t value;
    };
    struct Bar {
      struct Foo *fooptr;
    };
');

printXY($cdef->new('struct Foo'));
printXY($cdef->new('struct Bar'));

