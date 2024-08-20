<?php
require_once 'kphp_tester_include.php';

class MoneyRequest {
  private float $amount;

  public function __construct(float $amount) {
    $this->amount = $amount;
  }

  public function getAmount(): float {
    return $this->amount;
  }
}

function test_json_float_read(): void {
  $raw = '{"amount":101}';

  $res = JsonEncoder::decode($raw, MoneyRequest::class);
  var_dump($res->getAmount());
}

test_json_float_read();
