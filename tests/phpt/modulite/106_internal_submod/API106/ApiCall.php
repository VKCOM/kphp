<?php

namespace API106;

class ApiCall {
    static function makeCall(string $api_method_name) {
        Impl\ApiInternals::doSmth();
    }
}
