@ok
KPHP_ERROR_ON_WARNINGS=1
<?php

class Item {
  public function run() { return 10; }
}

class Example  {
  /** @var Item */
  private $item;

  /** @var ?Item */
  private $x;

  public function __construct() {
    $this->item = new Item();
  }

  /**
   * @kphp-required
   * @return array
   */
  public function run() {
    $this->item->run();

    $result = [
      'a' => $this->item->run(),
    ];

    if ($this->x !== null) {
      $this->x->run();
    }

    return $result;
  }
}

$example = new Example();
$example->run();
