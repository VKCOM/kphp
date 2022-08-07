<?php

namespace Feed005;

class SenderFactory {
    static function createSender(string $name): Send005\ISender {
        switch ($name) {
            case 'email': return new Send005\SenderEMail;
            case 'sms': return new Send005\SenderSms;
            default: throw new \Exception("invalid sender name");
        }
    }
}
