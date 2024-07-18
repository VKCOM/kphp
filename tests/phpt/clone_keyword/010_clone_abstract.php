@ok
<?php

abstract class Base {
}

class PurchaseAdd {
  public ?Reward1 $bonus_reward1 = null;
  public ?Reward2 $bonus_reward2 = null;

  function __construct(?Reward1 $bonus_reward1, ?Reward2 $bonus_reward2) {
    $this->bonus_reward1 = $bonus_reward1;
    $this->bonus_reward2 = $bonus_reward2;
  }

  public function __clone() {
    if ($this->bonus_reward1 !== null) {
      $this->bonus_reward1 = clone $this->bonus_reward1;
    }
    if ($this->bonus_reward2 !== null) {
      $this->bonus_reward2 = clone $this->bonus_reward2;
    }
  }
}

abstract class Reward1 {
  abstract function out();
}

interface Reward2 {
  function out();
}

final class PackReward1 extends Reward1 {
  public ?int $pack = null;

  function __construct(?int $pack) { $this->pack = $pack; }
  function out() { echo $this->pack, "\n"; }
}

final class PackReward2 implements Reward2 {
  public ?int $pack = null;

  function __construct(?int $pack) { $this->pack = $pack; }
  function out() { echo $this->pack, "\n"; }
}

final class DiscountReward1 extends Reward1 {
  public ?string $discount = null;

  function __construct(?string $discount) { $this->discount = $discount; }
  function out() { echo $this->discount, "!\n"; }
}

final class DiscountReward2 implements Reward2 {
  public ?string $discount = null;

  function __construct(?string $discount) { $this->discount = $discount; }
  function out() { echo $this->discount, "!\n"; }
}

function test1() {
    $add = clone (new PurchaseAdd(null, null));
    var_dump($add->bonus_reward1 === null);
    var_dump($add->bonus_reward2 === null);
}

function test2() {
    $orig1 = new PackReward1(10);
    $orig2 = new PackReward2(10);
    $add = clone (new PurchaseAdd($orig1, $orig2));
    $orig1->pack = 20;
    $orig2->pack = 20;
    $orig1->out();
    $orig2->out();
    $add->bonus_reward1->out();
    $add->bonus_reward2->out();
}

function test3() {
    $add = clone (new PurchaseAdd(new DiscountReward1('asdf'), new DiscountReward2('asdf')));
    $add->bonus_reward1->out();
    $add->bonus_reward2->out();
}

function clTest(Reward1 $r) {
    $r2 = clone $r;
    if ($r instanceof PackReward1) {
        $r->pack = 20;
    }
    $r2->out();
    $r->out();
}

test1();
test2();
test3();

clTest(new PackReward1(10));
