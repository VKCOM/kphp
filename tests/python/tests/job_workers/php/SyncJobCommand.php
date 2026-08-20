<?php

/** @kphp-immutable-class */
class SyncJobCommand {
  public $command = '';

  function __construct(string $command) {
    $this->command = $command;
  }
}
