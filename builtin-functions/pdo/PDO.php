<?php

require_once __DIR__ . '/PDOStatement.php';

class PDO {
    /**
     * Sets the timeout value in seconds for communications with the database.
     * @link https://php.net/manual/en/pdo.constants.php#pdo.constants.attr-timeout
     */
    const ATTR_TIMEOUT = 2;

    public function __construct(
        string $dsn,
        ?string $username = null,
        ?string $password = null,
        ?array $options = null
    ): PDO;

    /** @kphp-extern-func-info resumable */
    public function query(string $query): ?PDOStatement;    // TODO: $fetchMode is not supported

    /** @kphp-extern-func-info resumable */
    public function exec(string $statement): int|false;

    public function errorCode(): ?string;
    public function errorInfo(): (int|string)[];

//     These methods are not supported yet:
//
//     public function prepare(string $query, array $options = []): ?PDOStatement;
//     public function beginTransaction(): bool;
//     public function commit(): bool;

//     public function getAttribute(int $attribute): bool|int|string|array|null;
//     public function inTransaction(): bool;
//     public function lastInsertId(?string $name = null): string|false;
//     public static function getAvailableDrivers(): array;
//     public function quote(string $string, int $type = PDO::PARAM_STR): string|false;
//     public function rollBack(): bool;
//     public function setAttribute(int $attribute, mixed $value): bool;
}

