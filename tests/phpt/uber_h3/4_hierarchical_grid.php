@ok
<?php

require_once 'kphp_tester_include.php';

function test_h3ToParent() {
  assert_int_eq3(\UberH3::h3ToParent(0, -10), 0);
  assert_int_eq3(\UberH3::h3ToParent(0, 16), 0);

  assert_int_eq3(\UberH3::h3ToParent(0, 0), 0);
  assert_int_eq3(\UberH3::h3ToParent(614053082818412543, 10), 0);

  assert_int_eq3(\UberH3::h3ToParent(614053082818412543, 8), 614053082818412543);
  assert_int_eq3(\UberH3::h3ToParent(614053082818412543, 3), 591535125439709183);
}

function test_h3ToChildren() {
  assert_true(\UberH3::h3ToChildren(0, -10) === false);
  assert_true(\UberH3::h3ToChildren(0, 16) === false);

  assert_array_int_eq3(\UberH3::h3ToChildren(0, 0), [0]);

  assert_array_int_eq3(\UberH3::h3ToChildren(614053082818412543, 2), []);

  assert_array_int_eq3(\UberH3::h3ToChildren(614053082818412543, 8), [614053082818412543]);
  assert_array_int_eq3(\UberH3::h3ToChildren(614053082818412543, 9), [
    618556682443948031,
    618556682444210175,
    618556682444472319,
    618556682444734463,
    618556682444996607,
    618556682445258751,
    618556682445520895
  ]);
}

function test_maxH3ToChildrenSize() {
  assert_int_eq3(\UberH3::maxH3ToChildrenSize(0, -10), 0);
  assert_int_eq3(\UberH3::maxH3ToChildrenSize(0, 16), 0);

  assert_int_eq3(\UberH3::maxH3ToChildrenSize(0, 0), 1);

  assert_int_eq3(\UberH3::maxH3ToChildrenSize(614053082818412543, 2), 0);

  assert_int_eq3(\UberH3::maxH3ToChildrenSize(614053082818412543, 8), 1);
  assert_int_eq3(\UberH3::maxH3ToChildrenSize(614053082818412543, 9), 7);
}

function test_h3ToCenterChild() {
  assert_int_eq3(\UberH3::h3ToCenterChild(0, -10), 0);
  assert_int_eq3(\UberH3::h3ToCenterChild(0, 16), 0);

  assert_int_eq3(\UberH3::h3ToCenterChild(0, 0), 0);

  assert_int_eq3(\UberH3::h3ToCenterChild(614053082818412543, 2), 0);

  assert_int_eq3(\UberH3::h3ToCenterChild(614053082818412543, 8), 614053082818412543);
  assert_int_eq3(\UberH3::h3ToCenterChild(614053082818412543, 9), 618556682443948031);
}

function test_uncompact() {
  assert_true(\UberH3::uncompact([], -10) === false);
  assert_true(\UberH3::uncompact([], 16) === false);

  assert_array_int_eq3(\UberH3::uncompact([], 0), []);

  assert_true(\UberH3::uncompact([618556682444472319], 7) === false);
  assert_array_int_eq3(\UberH3::uncompact([618556682444472319], 9), [618556682444472319]);

  assert_array_int_eq3(\UberH3::uncompact([618556682444996607], 10), [
    623060282072137727,
    623060282072170495,
    623060282072203263,
    623060282072236031,
    623060282072268799,
    623060282072301567,
    623060282072334335
  ]);
}

function test_maxUncompactSize() {
  assert_int_eq3(\UberH3::maxUncompactSize([], -10), 0);
  assert_int_eq3(\UberH3::maxUncompactSize([], 16), 0);

  assert_int_eq3(\UberH3::maxUncompactSize([], 0), 0);

  assert_int_eq3(\UberH3::maxUncompactSize([618556682444472319], 7), -1);
  assert_int_eq3(\UberH3::maxUncompactSize([618556682444472319], 9), 1);

  assert_int_eq3(\UberH3::maxUncompactSize([618556682444996607], 10), 7);
}


test_h3ToParent();
test_h3ToChildren();
test_maxH3ToChildrenSize();
test_h3ToCenterChild();
test_uncompact();
test_maxUncompactSize();
