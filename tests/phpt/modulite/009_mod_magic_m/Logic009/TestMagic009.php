<?php

namespace Logic009;

class TestMagic009 {
    static function testToString() {
        $o = new \WithMagic009\WithToString009(1);
        echo "o $o\n";
    }

    static function testCtor() {
        new \GlobWithCtor009();
        \GlobWithCtor009::printInstancesCount();
    }

    static function testClone() {
        $o = new \GlobWithClone009;
        $o2 = clone $o;
        $o->printInfo();
        $o2->printInfo();
    }

    static function doSmth() {
    }
}
