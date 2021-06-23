@ok
KPHP_ERROR_ON_WARNINGS=1
<?php

class Options {
  public function add($k) {}
}

class Builder {
  /** @var ?int */
  public $id = null;

  public function build(Options $options = null) {
    if ($options !== null) {
      $options->add('x');
    }
    if ($this->id !== null) {
      if ($options !== null) {
        $options->add((string)$this->id);
      }
    }
    return 'result';
  }
}

function test() {
  $b = new Builder();
  $b->build(new Options());
  $b->build(null);
}

test();
