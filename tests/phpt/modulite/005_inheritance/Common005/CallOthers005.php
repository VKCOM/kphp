<?php

namespace Common005;

use \GloDer005;
use \Messages005\Message005;

class CallOthers005 {
    static public function accessGloDer() {
        // these members are actually in base class
        echo GloDer005::STR, "\n";
        echo GloDer005::$COUNT, "\n";
        GloDer005::stF();
    }

    static public function accessMessage() {
        echo Message005::ID, "\n";
        echo Message005::I_CNT, "\n";
        echo Message005::$NAME, "\n";
        echo Message005::getName(), "\n";
    }
}
