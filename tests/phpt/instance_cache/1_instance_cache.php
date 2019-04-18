@ok
<?php

require_once "instance_cache_stub.php";

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

class TreeX {
  /** @var int */
  public $value = 0;
  /** @var tuple<int,TreeX[]>[] */
  public $children = [];
}

class VectorY {
  /** @var Y[] */
  public $elements = [];
}

function test_empty_fetch() {
  $x = instance_cache_fetch(X::class, "key_x0");
  var_dump($x == false);
  $y = instance_cache_fetch(Y::class, "key_x0");
  var_dump($y == false);
}

function test_store_fetch() {
  var_dump(instance_cache_store("key_x1", new X));
  var_dump(instance_cache_store("key_y1", new Y(1, "test_store_fetch")));

  $x = instance_cache_fetch(X::class, "key_x1");
  $x->dump_me();

  $y = instance_cache_fetch(Y::class, "key_y1");
  $y->dump_me();
}

function test_mismatch_classes() {
  var_dump(instance_cache_store("key_x2", new X));
  var_dump(instance_cache_store("key_y2", new Y(2, "test_mismatch_classes", "optional")));

  $x = instance_cache_fetch(Y::class, "key_x2");
  var_dump($x == false);

  $y = instance_cache_fetch(X::class, "key_y2");
  var_dump($y == false);
}

function test_delete() {
  var_dump(instance_cache_store("key_x3", new X));
  var_dump(instance_cache_store("key_y3", new Y(3, "test_delete", "super optional")));

  var_dump(instance_cache_delete("key_x3_unknown"));
  var_dump(instance_cache_delete("key_y3_unknown"));

  $x = instance_cache_fetch(X::class, "key_x3");
  $x->dump_me();

  $y = instance_cache_fetch(Y::class, "key_y3");
  $x->dump_me();

  var_dump(instance_cache_delete("key_x3"));
  var_dump(instance_cache_delete("key_y3"));

  $x = instance_cache_fetch(X::class, "key_x3");
  var_dump($x == false);

  $y = instance_cache_fetch(Y::class, "key_y3");
  var_dump($y == false);

  var_dump(instance_cache_delete("key_x3"));
  var_dump(instance_cache_delete("key_y3"));
}

function test_clear() {
  var_dump(instance_cache_store("key_x4", new X));
  var_dump(instance_cache_store("key_y4", new Y(3, "test_clear")));

  $x = instance_cache_fetch(X::class, "key_x4");
  $x->dump_me();

  $y = instance_cache_fetch(Y::class, "key_y4");
  $x->dump_me();

  instance_cache_clear();

  $x = instance_cache_fetch(X::class, "key_x4");
  var_dump($x == false);

  $y = instance_cache_fetch(Y::class, "key_y4");
  var_dump($y == false);
}

function test_tree() {
  $root = new TreeX;
  $root->value = 0;
  $child = new TreeX;
  $child->value = 1;
  $root->children = [tuple(1, [$child])];

  var_dump(instance_cache_store("tree_root", $root));

  $cached_root1 = instance_cache_fetch(TreeX::class, "tree_root");
  assert_equal($cached_root1->value, 0);
  /**@var TreeX[]*/
  $children1 = $cached_root1->children[0][1];
  assert_equal(count($children1), 1);
  assert_equal($children1[0]->value, 1);
  assert_equal(count($children1[0]->children), 0);

  $root->value = 1;
    /**@var TreeX[]*/
  $children0 = $root->children[0][1];
  $children0[0]->value = 2;
  $children0[] = new TreeX;
  $children0[1]->value = 3;

  assert_equal($cached_root1->value, 0);
  /**@var TreeX[]*/
  $children1 = $cached_root1->children[0][1];
  assert_equal(count($children1), 1);
  assert_equal($children1[0]->value, 1);
  assert_equal(count($children1[0]->children), 0);

  $cached_root2 = instance_cache_fetch(TreeX::class, "tree_root");
  assert_equal($cached_root2->value, 0);
  /**@var TreeX[]*/
  $children2 = $cached_root2->children[0][1];
  var_dump(count($children2));
  assert_equal(count($children2), 1);
  assert_equal($children2[0]->value, 1);
  assert_equal(count($children2[0]->children), 0);
}

function test_loop_in_tree() {
  $root = new TreeX;
  $root->value = 0;
  $root->children = [tuple(0, [$root])];

  $result = instance_cache_store("tree_root_loop", $root);
  assert_equal($result, false);

  $cached_root1 = instance_cache_fetch(TreeX::class, "tree_root_loop");
  assert_equal($cached_root1 == false, true);
}

function test_same_instance_in_array() {
  $vector = new VectorY;
  $y = new Y(10, " <-first");
  $vector->elements = [$y, $y, new Y(11, " <-second")];

  var_dump(instance_cache_store("vector", $vector));

  $cached_vector = instance_cache_fetch(VectorY::class, "vector");
  assert_equal($cached_vector->elements[0]->y_string, "hello world <-first");
  assert_equal($cached_vector->elements[1]->y_string, "hello world <-first");
  assert_equal($cached_vector->elements[2]->y_string, "hello world <-second");

  $cached_vector->elements[1]->y_string = "hello world <-second";
  $cached_vector->elements[2]->y_string = "hello world <-third";
  assert_equal($cached_vector->elements[0]->y_string, "hello world <-first");
  assert_equal($cached_vector->elements[1]->y_string, "hello world <-second");
  assert_equal($cached_vector->elements[2]->y_string, "hello world <-third");
}

function test_memory_limit_exceed() {
  $vector = new VectorY;
  for ($i = 1; $i < 100000; ++$i) {
    $vector->elements[] = new Y($i, " <-");
  }

  assert_equal(instance_cache_store("large_vector", $vector) == false, true);
  assert_equal(instance_cache_fetch(VectorY::class, "large_vector") == false, true);
}

test_empty_fetch();
test_store_fetch() ;
test_mismatch_classes();
test_delete();
test_clear();
test_tree();
test_loop_in_tree();
test_same_instance_in_array();
// this test should be the last!
test_memory_limit_exceed();