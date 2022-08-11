<?php

namespace Engines\Messages\Folders;

use Engines\Messages\Core\ClusterConnection;
use Engines\Messages\Core\MsgLogger;

class FoldersRep {
    private ?ClusterConnection $conn = null;

    static public function createFoldersRep(): self {
        return new self;
    }

    public function makeConnection(): self {
        $this->conn = new ClusterConnection();
        return $this;
    }

    public function log(): self {
        MsgLogger::logMessage($this->conn);
        return $this;
    }
}
