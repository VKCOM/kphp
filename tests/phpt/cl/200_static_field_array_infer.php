@ok
<?php

/*
 * This test checks that the type of $filtered_ids_by_reason's init value
 * is inferred as array<int>. Previously, it could be inferred as
 * array<Unknown>.
 *
 * With array<Unknown>, a conversion to array<int> would be required, which
 * is not allowed in static variable initialization: the init value must
 * have exactly the same C++ type as the field itself.
 */

class SearchFilter {
  public const REASON_A = 'reason_a';
  public const REASON_B = 'reason_b';
  public const REASON_C = 'reason_c';
  public const REASON_D = 'reason_d';

  /** @var int[][] */
  private static $filtered_ids_by_reason = [
    self::REASON_A => [],
    self::REASON_B => [],
    self::REASON_C => [],
    self::REASON_D => [],
  ];
}

var_dump(new SearchFilter === new SearchFilter);
