<?php

namespace Api;

class ApiClient {
    public function request(string $endpoint): string {
        return "GET " . $endpoint;
    }
}
