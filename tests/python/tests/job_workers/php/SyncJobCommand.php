<?php

/** @kphp-immutable-class
 *  @kphp-serializable */
class SyncJobCommand {
  /** @kphp-serialized-field 0 */
  public $command = '';

  function __construct(string $command) {
    $this->command = $command;
  }
}
