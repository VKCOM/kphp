<?php

namespace Feed005\Send005;

abstract class SenderBaseImpl {
    abstract function getDest(): string;

    function printLocation() {
        echo "location: ", $this->getDest(), "\n";
    }
}
