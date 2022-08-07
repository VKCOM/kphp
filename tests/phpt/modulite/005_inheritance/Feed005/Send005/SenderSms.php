<?php

namespace Feed005\Send005;

class SenderSms extends SenderBaseImpl implements ISender {
    function send() {
        $this->printLocation();
    }

    function getDest(): string {
        return "sms";
    }
}
