<?php

namespace Logic008;

class A {
    public function fa() { echo "fa\n"; }
}

class TestLogic008 {
    static function testMapper() {
        $in = [new A];
        $out = my_map($in, fn($a) => clone $a);
        foreach ($out as $a)
            $a->fa();
    }

    static function printEitherIsError(string $prefix, object $either) {
        if (\Either008::isError($either))
            echo "$prefix: is error\n";
        else
            echo "$prefix: not error\n";

        \internal_f_non_generic('t1');
        \internal_f_generic('t2', $either);
    }
}
