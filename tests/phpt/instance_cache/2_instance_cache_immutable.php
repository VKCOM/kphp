@ok
<?php

require_once 'polyfills.php';

/** @kphp-immutable-class */
class X {
  /** @var int */
  public $x_int = 1;
  /** @var string */
  public $x_str = "hello";
  /** @var int[] */
  public $x_array = [1, 2, 3, 4];
  /** @var array */
  public $x_array_var = ["foo", 12, [123]];
}

/** @kphp-immutable-class */
class Y {
  /** @var X */
  public $x_instance;
  /** @var string */
  public $y_string;
  /** @var int[] */
  public $y_array;
  /** @var array */
  public $y_array_var;
  /** @var string|false */
  public $y_string_or_false;
  /** @var tuple<string|false, array, int[], string, X> */
  public $y_tuple;

  public function __construct($i, $s, $or_false_str = false) {
    $this->x_instance = new X;
    $this->y_string = $this->x_instance->x_str . " world" . $s;
    $this->y_array = $this->x_instance->x_array;
    $this->y_array[] = $i;
    $this->y_array_var = $this->x_instance->x_array_var;
    $this->y_array_var[] = $s;
    $this->y_string_or_false = 1 ? "or_false" : false;
    $this->y_tuple = tuple($or_false_str, $this->y_array_var, $this->y_array, $this->y_string, new X);
  }
}

/** @kphp-immutable-class */
class TreeX {
  public function __construct($v, $c) {
    $this->value = $v;
    $this->children = $c;
  }
  /** @var int */
  public $value = 0;
  /** @var tuple<int, TreeX[]>[] */
  public $children = [];
}

/** @kphp-immutable-class */
class VectorY {
  public function __construct($e) {
    $this->elements = $e;
  }

  /** @var Y[] */
  public $elements = [];
}

function test_store_fetch() {
  var_dump(instance_cache_store("key_x1", new X));
  var_dump(instance_cache_store("key_y1", new Y(1, "test_store_fetch")));

  $x = instance_cache_fetch_immutable(X::class, "key_x1");
  var_dump(instance_to_array($x));

  $y = instance_cache_fetch_immutable(Y::class, "key_y1");
  var_dump(instance_to_array($y));
}

function test_tree() {
  $child = new TreeX(1, []);
  $root = new TreeX(0, [tuple(1, [$child])]);

  var_dump(instance_cache_store("tree_root", $root));

  $cached_root1 = instance_cache_fetch_immutable(TreeX::class, "tree_root");
  var_dump(instance_to_array($cached_root1));
}

function test_same_instance_in_array() {
  $y = new Y(10, " <-first");
  $vector = new VectorY([$y, $y, new Y(11, " <-second")]);

  var_dump(instance_cache_store("vector", $vector));

  $cached_vector = instance_cache_fetch_immutable(VectorY::class, "vector");
  var_dump(instance_to_array($cached_vector));
}

test_store_fetch();
test_tree();
test_same_instance_in_array();
