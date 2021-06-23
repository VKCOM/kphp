@ok php7_4
<?php

// More than 4 prop smart casts, but through different vars.

function test(Foo $x, Foo $y) {
  if ($x->p1) {
    nonnull_bar1($x->p1);
    if ($x->p2) {
      nonnull_bar1($x->p2);
      if ($x->p3) {
        nonnull_bar1($x->p3);
        if ($x->p4) {
          nonnull_bar1($x->p4);
        }
      }
    }
  }

  if ($y->p1) {
    nonnull_bar1($y->p1);
    if ($y->p2) {
      nonnull_bar1($y->p2);
      if ($y->p3) {
        nonnull_bar1($y->p3);
        if ($y->p4) {
          nonnull_bar1($y->p4);
        }
      }
    }
  }
}

function nonnull_bar1(Bar $bar) {}
function nonnull_bar2(Bar $bar) {}

class Foo {
  public ?Bar $p1 = null;
  public ?Bar $p2 = null;
  public ?Bar $p3 = null;
  public ?Bar $p4 = null;
  public ?Bar $p5 = null;
}

class Bar {}

test(new Foo(), new Foo());
