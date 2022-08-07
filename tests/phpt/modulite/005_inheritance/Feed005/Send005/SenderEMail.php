<?php

namespace Feed005\Send005;

class SenderEMail extends SenderBaseImpl implements ISender {
    function send() {
        $this->printLocation();
    }

    function getDest(): string {
        return "email";
    }
}
