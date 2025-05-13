<?php

function main() {
  switch ($_SERVER["PHP_SELF"]) {
    case "/store": {
      test_store();
      return;
    }
    case "/fetch_and_verify": {
      test_fetch_and_verify();
      return;
    }
    case "/delete": {
      test_delete();
      return;
    }
  }

  critical_error("unknown test " . $_SERVER["PHP_SELF"]);
}

/** @kphp-immutable-class
 *  @kphp-serializable
 */
class TreeNode {
  /** @var int
   *  @kphp-serialized-field 0
   */
  public $val;

  /** @var TreeNode
   *  @kphp-serialized-field 1
   */
  public $left;
  
  /** @var TreeNode
   *  @kphp-serialized-field 2
   */
  public $right;
  
  public function __construct(int $depth) {
      $this->val = rand(1, 100);
      if (rand(1, $depth) > 1) {
        $this->left = new TreeNode($depth - 1);
      }
      if (rand(1, $depth) > 1) {
        $this->right = new TreeNode($depth - 1);
      }
  }
}

/** @kphp-immutable-class
 *  @kphp-serializable
 */
class SerializedTree {
  /** @var TreeNode
   *  @kphp-serialized-field 0
   */
  public $node;

  /** @kphp-serialized-field 1 */
  public string $serialized;

  public function __construct(int $depth) {
    $this->node = new TreeNode($depth);
    $this->serialized = serialize_tree($this->node);
  }

  public function is_valid() {
    return $this->serialized == serialize_tree($this->node); 
  }
}

/** @param TreeNode $root */
function serialize_tree($root) {
  if ($root === null) {
      return 'null';
  }
  
  $left = serialize_tree($root->left);
  $right = serialize_tree($root->right);
  
  return $root->val . ',' . $left . ',' . $right;
}

/**
 * @param $nodes string[]
 * @param $index int 
 * @return tuple(TreeNode, int) */
function build_tree($nodes, $index) {
  if ($index >= count($nodes)) {
    return tuple(null, $index);
  }
  
  $val = $nodes[$index];
  if ($val === 'null') {
      return tuple(null, $index + 1);
  }
  
  $node = new TreeNode((int)$val);
  list($node->left, $index) = build_tree($nodes, $index + 1);
  list($node->right, $index) = build_tree($nodes, $index);
  
  return tuple($node, $index);
}

function test_store() {
  $val = new SerializedTree(rand(0, 7));

  $data = json_decode(file_get_contents('php://input'));
  echo json_encode(["result" => instance_cache_store((string)$data["key"], $val, 5)]);
}

function test_fetch_and_verify() {
  $data = json_decode(file_get_contents('php://input'));
  /** @var SerializedTree $instance */
  $instance = instance_cache_fetch(SerializedTree::class, (string)$data["key"]);
  echo json_encode([
    "is_valid" => is_null($instance) || $instance->is_valid()
  ]);
}

function test_delete() {
  $data = json_decode(file_get_contents('php://input'));
  instance_cache_delete((string)$data["key"]);
}

main();
