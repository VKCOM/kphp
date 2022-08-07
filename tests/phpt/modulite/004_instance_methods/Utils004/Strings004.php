<?php

namespace Utils004;

class Strings004 {
    public int $count = 0;

    public function strlen(string $s) {
        return strlen($s);
    }

    static public function demoAccessHidden() {
        // this is Hidden004 — not exported
        // but we don't check calling instance methods and accessing instance properties
        // so, that is okay
        // BTW, this is not okay: /** @var \Messages004\Hidden004 $hidden */
        // because we can't access that class
        // as a consequence, we can't accept it as a parameter
        // (in practice, that class should be exported, logically)
        $hidden = \Messages004\Messages004::getHidden();
        $hidden->incId();
        echo $hidden->id, "\n";

        // that's also ok, we don't check instance methods
        $msg = new \Messages004\Messages004;
        $msg->instanceFnMarkedForceInternal();
    }
}
