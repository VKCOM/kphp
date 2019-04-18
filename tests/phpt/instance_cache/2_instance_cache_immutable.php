@ok
<?php

require_once "instance_cache_stub.php";

/** @kphp-immutable-class */
class X {
  public function dump_me() {
    var_dump($this->x_int);
    var_dump($this->x_str);
    var_dump($this->x_array);
    var_dump($this->x_array_var);
  }

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
  /** @var tuple<string|false,array,int[],string,X> */
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

  public function dump_me() {
    $this->x_instance->dump_me();
    var_dump($this->y_string);
    var_dump($this->y_array);
    var_dump($this->y_array_var);
    var_dump($this->y_string_or_false);
    var_dump($this->y_tuple[0]);
    var_dump($this->y_tuple[1]);
    var_dump($this->y_tuple[2]);
    var_dump($this->y_tuple[3]);
    /** @var X */
    $x = $this->y_tuple[4];
    $x->dump_me();
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
  /** @var tuple<int,TreeX[]>[] */
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
  $x->dump_me();

  $y = instance_cache_fetch_immutable(Y::class, "key_y1");
  $y->dump_me();
}

function test_tree() {
  $child = new TreeX(1, []);
  $root = new TreeX(0, [tuple(1, [$child])]);

  var_dump(instance_cache_store("tree_root", $root));

  $cached_root1 = instance_cache_fetch_immutable(TreeX::class, "tree_root");
  assert_equal($cached_root1->value, 0);
  /**@var TreeX[]*/
  $children1 = $cached_root1->children[0][1];
  assert_equal(count($children1), 1);
  assert_equal($children1[0]->value, 1);
  assert_equal(count($children1[0]->children), 0);
}

function test_same_instance_in_array() {
  $y = new Y(10, " <-first");
  $vector = new VectorY([$y, $y, new Y(11, " <-second")]);

  var_dump(instance_cache_store("vector", $vector));

  $cached_vector = instance_cache_fetch_immutable(VectorY::class, "vector");
  assert_equal($cached_vector->elements[0]->y_string, "hello world <-first");
  assert_equal($cached_vector->elements[1]->y_string, "hello world <-first");
  assert_equal($cached_vector->elements[2]->y_string, "hello world <-second");
}

test_store_fetch();
test_tree();
test_same_instance_in_array();
