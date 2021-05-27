<?php

namespace ComplexScenario;

interface StatsInterface {
  /**
   * @return mixed
   */
  function start_processing(int $master_port);

  /**
   * @param mixed $processing_id
   */
  function finish_processing($processing_id): Stats;
}
