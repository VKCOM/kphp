<?php

require_once __DIR__ . '/PDO.php';

class PDOStatement {
    /**
     * @kphp-const
     */
    public string $queryString;

    // TODO: что делать асинхронным через епол? коннект + запрос + ответ или только ответ?
    // - send_query и fetch_result должны быть асинхронными через epoll. Коннект создается лениво, либо в event_loop, либо перед инициированием посылки запроса.
    // На момент посылки запроса он уже есть.
    // 1. Делаем лениво connect. Когда создался connection, добавляем в реактор fd на ЗАПИСЬ.
    // 2. Когда готов к записи, передобавляем в реактор на ЧТЕНИЕ, запускаем отправку запроса (которая была отложенной).
    // 3. Когда готов к чтению - аналог read_job_results.

    // Как реализовать серверный стриминг?
    // Видимо так:
    // Будем эмулировать через simple resumable.
    // Будет resumable next_batch, он будет создаваться как forked из посылки запроса.
    // В handler'е обработки EPOLLIN:
    //     1. Фетчим все запросы (строки) которые готовы, сохраняем в контекст конекта.
    //     2. Если остались еще запросы (строки) от сервера, которые не пришли, то
    //         создаем новый forked mysql_next_batch resumable, сохраняем его id в контекст конекта (старый кладем в net_event, он завершится при process_net_events).
    // При вызове fetch мы смотрим: если строка которую мы хотим получить еще не готова, то прерываемся по ожиданию mysql_next_batch. Если готова, просто берем ее
    public function fetch(int $mode = PDO::FETCH_DEFAULT, int $cursorOrientation = PDO::FETCH_ORI_NEXT, int $cursorOffset = 0): mixed;

    /** @kphp-extern-func-info resumable */
    public function execute(?array $params = null): bool;

//     These methods are not supported yet:
//
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
