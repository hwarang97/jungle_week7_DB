#!/usr/bin/env bash

set -e
cd "$(dirname "$0")"

echo "[cycle3-demo] cleaning previous demo fixture"
rm -f data/schema/users_idx_demo.schema data/tables/users_idx_demo.csv

echo "[cycle3-demo] building sqlparser"
make sqlparser

echo "[cycle3-demo] running demo with select path trace"
STORAGE_TRACE_SELECT=1 ./sqlparser docs/cycle3-index-demo.sql
