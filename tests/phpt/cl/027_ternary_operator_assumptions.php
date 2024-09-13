@ok k2_skip
<?php

interface AI {
  public function say_hello();
}

class A1 implements AI {
  public function say_hello() {
    echo "hello A1\n";
  }
}

class A2 implements AI {
  public function say_hello() {
    echo "hello A2\n";
  }
}

class B {
  /**
   * @param int $x
   * @return AI
   */
  public function make_a($x): AI {
    return $x == 1 ? new A1 : new A2;
  }
}

function test_ternary_operator(?B $b, bool $f) {
  $factory = $b ? $b : null;
  if ($factory) {
    $a = $f ? $factory->make_a(1) : $factory->make_a(2);
    $a->say_hello();
  }
}

function test_ternary_operator_short(?B $b) {
  $factory = $b ?: null;
  if ($factory) {
    $a1 = $factory->make_a(1);
    $a = $a1 ?: $factory->make_a(2);
    $a->say_hello();
  }
}

class Group {
    public int $id = 1;
}

function getGroup() { return new Group; }

function test_ternary_with_null_1() {
    $best_group = [[new Group]][0][0];   // too difficult for assumptions, would be undefined
    $group = new Group;

    $r1 = $best_group ?: $group;
    $r2 = $best_group ? $best_group : $group;
    $r3 = getGroup() ?: $group;
    $r4 = getGroup() ?: null;

    echo $r1->id, $r2->id, $r3->id, $r4->id, "\n";
}

function test_ternary_with_null_2() {
    $best_group = new Group;
    $group = null;

    $result_group_1 = $best_group ?: $group;
    $result_group_2 = $best_group ? $best_group : $group;

    echo $result_group_1->id + $result_group_2->id, "\n";
}

function test_ternary_recursive_1() {
    $best_group = 1 ? null : new Group;
    $group = new Group;

    $best_group = $best_group ?: $group;
    echo $best_group->id, "\n";
}

function test_ternary_recursive_2() {
    $best_group = new Group;
    $group = null;

    $best_group = $best_group ?: $group;
    echo $best_group->id, "\n";
}

function test_ternary_recursive_3() {
    $best_group = [[new Group]][0][0];   // too difficult for assumptions, would be undefined
    $group = new Group;

    $best_group = $best_group ? $best_group : $group;
    echo $best_group->id, "\n";
}

function test_null_coalesce_1() {
    $best_group = null;
    $group = new Group;

    $r1 = $best_group ?? $group;
    $r2 = getGroup() ?? $group;
    $r3 = $group ?? getGroup();
    $r4 = $group ?? $best_group;

    echo $r1->id, $r2->id, $r3->id, $r4->id, "\n";
}

test_ternary_operator(new B, true);
test_ternary_operator(new B, false);
test_ternary_operator(null, true);

test_ternary_operator_short(new B);
test_ternary_operator_short(null);

test_ternary_with_null_1();
test_ternary_with_null_2();
test_ternary_recursive_1();
test_ternary_recursive_2();
test_ternary_recursive_3();
test_null_coalesce_1();
