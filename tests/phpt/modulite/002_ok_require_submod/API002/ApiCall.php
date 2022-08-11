<?php

namespace API002;

class ApiCall {
    static function makeCall(string $api_method_name) {
        Impl\ApiInternals::doSmth();
        (new \Glob002)->doSmth();
    }
}
