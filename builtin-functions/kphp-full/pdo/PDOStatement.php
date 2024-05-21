<?php

require_once __DIR__ . '/PDO.php';

class PDOStatement {
    /**
     * @kphp-const
     */
    public string $queryString;

    public function fetch(): mixed;      // TODO: only default behaviour supported
    public function fetchAll(): mixed[]; // TODO: only default behaviour supported

//     These methods are not supported yet:
//
//     public function execute(?array $params = null): bool;
//     public bindColumn(
//         string|int $column,
//         mixed &$var,
//         int $type = PDO::PARAM_STR,
//         int $maxLength = 0,
//         mixed $driverOptions = null
//     ): bool
//     public bindParam(
//         string|int $param,
//         mixed &$var,
//         int $type = PDO::PARAM_STR,
//         int $maxLength = 0,
//         mixed $driverOptions = null
//     ): bool
//     public bindValue(string|int $param, mixed $value, int $type = PDO::PARAM_STR): bool
//     public closeCursor(): bool
//     public columnCount(): int
//     public debugDumpParams(): ?bool
//     public errorCode(): ?string
//     public errorInfo(): array
//     public fetchAll(int $mode = PDO::FETCH_DEFAULT): array
//     public fetchColumn(int $column = 0): mixed
//     public fetchObject(?string $class = "stdClass", array $constructorArgs = []): object|false
//     public getAttribute(int $name): mixed
//     public getColumnMeta(int $column): array|false
//     public nextRowset(): bool
//     public rowCount(): int
//     public setAttribute(int $attribute, mixed $value): bool
//     public setFetchMode(int $mode): bool
}
