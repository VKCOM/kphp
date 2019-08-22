@ok
<?php

require_once "polyfills.php";

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
  var_dump(instance_to_array($x));

  $y = instance_cache_fetch(Y::class, "key_y1");
  var_dump(instance_to_array($y));
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
  var_dump(instance_to_array($x));

  $y = instance_cache_fetch(Y::class, "key_y3");
  var_dump(instance_to_array($y));

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
  var_dump(instance_to_array($x));

  $y = instance_cache_fetch(Y::class, "key_y4");
  var_dump(instance_to_array($y));

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

  var_dump(instance_to_array($cached_root1));

#ifndef KittenPHP
  if (false)
#endif
  $root->value = 1;

  var_dump(instance_to_array($cached_root1));

  $cached_root2 = instance_cache_fetch(TreeX::class, "tree_root");
  var_dump(instance_to_array($cached_root2));
}

function test_loop_in_tree() {
  $root = new TreeX;
  $root->value = 0;
  $root->children = [tuple(0, [$root])];

  $result = instance_cache_store("tree_root_loop", $root);
#ifndef KittenPHP
  var_dump(false);
  if (false)
#endif
  var_dump($result);

  $cached_root1 = instance_cache_fetch(TreeX::class, "tree_root_loop");
#ifndef KittenPHP
  var_dump(true);
  if (false)
#endif
  var_dump($cached_root1 == false);
}

function test_same_instance_in_array() {
  $vector = new VectorY;
  $y = new Y(10, " <-first");
#ifndef KittenPHP
  $vector->elements = [$y, clone $y, new Y(11, " <-second")];
  if (false)
#endif
  $vector->elements = [$y, $y, new Y(11, " <-second")];

  var_dump(instance_cache_store("vector", $vector));

  $cached_vector = instance_cache_fetch(VectorY::class, "vector");
  var_dump(instance_to_array($cached_vector));

  $cached_vector->elements[1]->y_string = "hello world <-second";
  $cached_vector->elements[2]->y_string = "hello world <-third";
  var_dump(instance_to_array($cached_vector));
}

function test_memory_limit_exceed() {
  $vector = new VectorY;
  for ($i = 1; $i < 1000000; ++$i) {
    $vector->elements[] = new Y($i, " <-");
  }

#ifndef KittenPHP
  var_dump(false);
  if (false)
#endif
  var_dump(instance_cache_store("large_vector", $vector));
  var_dump(instance_cache_fetch(VectorY::class, "large_vector") == false);
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