@ok
<?php

class Example {
  public $v = 0;
  public function __construct($v) { $this->v = $v; }
}

function test() {
	$xs = [new Example(1), new Example(4)];
	foreach ($xs as $x) {
		while (1) {
			throw new Exception("$x->v");
		}
	    return;
	}
}

try {
	test();
} catch (Exception $e) {
	var_dump($e->getMessage());
}
