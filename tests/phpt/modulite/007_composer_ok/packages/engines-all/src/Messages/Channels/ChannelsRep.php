<?php

namespace Engines\Messages\Channels;

use Engines\Messages\Core\ClusterConnection;
use Engines\Messages\Core\MsgLogger;

class ChannelsRep {
    private ?ClusterConnection $conn = null;

    static public function createChannelsRep(): self {
        return new self;
    }

    public function makeConnection(): self {
        $this->conn = new ClusterConnection();
        return $this;
    }

    public function log(): self {
        MsgLogger::logMessageInternal($this->conn);
        return $this;
    }

    public function logWith(string $postfix): self {
        MsgLogger::logMessageInternal($this->conn, $postfix);
        return $this;
    }
}
