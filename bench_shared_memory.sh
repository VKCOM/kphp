#!/usr/bin/env bash
# bench_shared_memory.sh — ramp load test for f$shared_memory_test().
#
# Идея: гоним растущий offered rate. Если внутри стоит глобальный RW-lock,
# реальный throughput (Requests/rate) упрётся в потолок и перестанет расти,
# а latency p99 взлетит. Параллельно смотрим ns_per_op в логах инстанса:
# рост ns_per_op под нагрузкой = contention на локе.

set -euo pipefail

TARGET="GET http://localhost:8000/"
DURATION="${DURATION:-10s}"
RATES="${RATES:-50 75 80 90 100 120 150 200 250 300}"

printf '%-8s %-12s %-12s %-12s %-12s %-10s\n' "rate" "throughput" "mean" "p50" "p99" "success"
for rate in $RATES; do
  report=$(echo "$TARGET" | vegeta attack -rate "$rate" -duration "$DURATION" \
           | vegeta report -type json)
  thr=$(echo "$report" | jq -r '.throughput | . * 100 | round / 100')
  mean=$(echo "$report" | jq -r '.latencies.mean / 1e6 | . * 100 | round / 100')
  p50=$(echo "$report" | jq -r '.latencies."50th" / 1e6 | . * 100 | round / 100')
  p99=$(echo "$report" | jq -r '.latencies."99th" / 1e6 | . * 100 | round / 100')
  succ=$(echo "$report" | jq -r '.success * 100 | round')
  printf '%-8s %-12s %-12s %-12s %-12s %-10s\n' \
    "$rate" "$thr" "${mean}ms" "${p50}ms" "${p99}ms" "${succ}%"
done

