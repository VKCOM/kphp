@ok
KPHP_ERROR_ON_WARNINGS=1
<?php

class Options {
  public function add($k, $x) {}
}

class Builder {
  /** @var ?int */
  public $id = null;

  /** @var ?(int[]) */
  public $counters = null;

  public function build(Options $options = null) {
    $result = [
      'id1' => $this->id,
    ];

    if ($options !== null) {
      $options->add('id1', $this->id);
    }

    if ($this->id !== null) {
      if ($options !== null) {
        $options->add('id2', $this->id);
      }
      $result['id2'] = $this->id !== null ? $this->id : 0;
    }

    if ($this->counters !== null) {
      $result['counters'] = [];
      foreach ($this->counters as $x) {
        $result['counters'][] = ['item' => $x];
      }
    }

    return $result;
  }
}

function test() {
  $b = new Builder();
  $b->counters = [1, 2, 3];
  var_dump($b->build(new Options()));
  var_dump($b->build(null));
}

test();
