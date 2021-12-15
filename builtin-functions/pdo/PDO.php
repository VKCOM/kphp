<?php

require_once __DIR__ . '/PDOStatement.php';

class PDO {
    const FETCH_DEFAULT = 42;
    const FETCH_ORI_NEXT = 42;

    public function __construct(
        string $dsn,
        ?string $username = null,
        ?string $password = null,
        ?array $options = null
    ): PDO;

    /** @kphp-extern-func-info resumable */
    public function query(string $query, ?int $fetchMode = null): ?PDOStatement;
    public function prepare(string $query, array $options = []): ?PDOStatement;

//     These methods are not supported yet:
//
//     public function beginTransaction(): bool;
//     public function commit(): bool;
//     public function errorCode(): ?string;
//     public function errorInfo(): array;
//     public function getAttribute(int $attribute): bool|int|string|array|null;
//     public function inTransaction(): bool;
//     public function lastInsertId(?string $name = null): string|false;
//     public function exec(string $statement): int|false;
//     public static function getAvailableDrivers(): array;
//     public function quote(string $string, int $type = PDO::PARAM_STR): string|false;
//     public function rollBack(): bool;
//     public function setAttribute(int $attribute, mixed $value): bool;
}

