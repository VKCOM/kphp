<?php

namespace Messages106;

class Messages106 {
    static function act() {
        Channels106\Channels106::doSmth();
        Channels106\Infra106\Infra106::doSmth();
        Core106\Core106::doSmth();
        \API106\ApiCall::makeCall('');
    }
}
