<?php

namespace ComplexScenario;

class CollectStatsJobResponse implements \KphpJobWorkerResponse {
  /** @var ?Stats */
  public $stats = null;

  /** @var int[] */
  public $workers_pids = [];

  /** @var ?NetPid */
  public $server_net_pid = null;
}
