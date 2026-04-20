#!/usr/bin/env python3
"""Fetch live prices from Stooq and update a portfolio CSV file.

Usage: python refresh_prices.py <csv_path>
Writes a JSON status file at <csv_path>.refresh_status on completion.
"""
import csv
import datetime
import json
import os
import sys
from urllib.error import URLError
from urllib.request import Request, urlopen

# Explicit symbol → Stooq symbol mapping for non-US tickers.
# US stocks are tried with .us suffix automatically (e.g. AAPL → aapl.us).
SUFFIX_MAP: dict[str, str] = {
    "AIR":  "AIR.DE",   # Airbus — XETRA (Stooq has no Euronext Paris data)
    "SXR8": "SXR8.DE",  # iShares S&P 500 — XETRA
    "IWDA": "IWDA.AS",  # iShares MSCI World — Amsterdam
    "EUNL": "EUNL.DE",  # iShares Core MSCI World — XETRA
    "VUSA": "VUSA.AS",  # Vanguard S&P 500 — Amsterdam
    "CSPX": "CSPX.AS",  # iShares Core S&P 500 — Amsterdam
    "VWCE": "VWCE.DE",  # Vanguard FTSE All-World — XETRA
    "IMAE": "IMAE.AS",  # iShares MSCI EM — Amsterdam
    "IUSA": "IUSA.AS",  # iShares S&P 500 — Amsterdam
    "VUAA": "VUAA.DE", 
}


def _fetch_stooq(stooq_sym: str) -> float | None:
    url = f"https://stooq.com/q/l/?s={stooq_sym.lower()}&f=sd2t2ohlcvn&h&e=csv"
    try:
        req = Request(url, headers={"User-Agent": "Mozilla/5.0"})
        with urlopen(req, timeout=15) as resp:
            text = resp.read().decode("utf-8", errors="replace")
            rows = list(csv.reader(text.splitlines()))
            if len(rows) < 2:
                return None
            headers = [h.strip() for h in rows[0]]
            try:
                close_idx = headers.index("Close")
            except ValueError:
                close_idx = 6 if len(headers) > 6 else None
            if close_idx is None:
                return None
            for row in rows[1:]:
                if len(row) > close_idx:
                    val = row[close_idx].strip()
                    if val and val.upper() not in ("N/D", "N/A", "-"):
                        return float(val)
    except (URLError, ValueError, IndexError, OSError):
        pass
    return None


def fetch_price(symbol: str) -> float | None:
    sym_upper = symbol.upper()

    # If we have an explicit mapping, use it directly.
    if sym_upper in SUFFIX_MAP:
        return _fetch_stooq(SUFFIX_MAP[sym_upper])

    # Otherwise try with .us suffix first (most common case for US stocks),
    # then fall back to the plain symbol.
    for candidate in (sym_upper + ".US", sym_upper):
        price = _fetch_stooq(candidate)
        if price is not None:
            return price
    return None


def main() -> None:
    if len(sys.argv) < 2:
        result = {"error": "No CSV path provided", "updated": 0, "failed": [], "ts": ""}
        print(json.dumps(result))
        sys.exit(1)

    csv_path = sys.argv[1]
    if not os.path.exists(csv_path):
        result = {"error": f"File not found: {csv_path}", "updated": 0, "failed": [], "ts": ""}
        print(json.dumps(result))
        sys.exit(1)

    with open(csv_path, "r", encoding="utf-8") as f:
        content = f.read()

    lines = content.splitlines()

    # Collect symbols from [stocks] section
    symbols: list[str] = []
    in_stocks = False
    for line in lines:
        stripped = line.strip()
        if stripped == "[stocks]":
            in_stocks = True
            continue
        if stripped.startswith("[") and stripped != "[stocks]":
            in_stocks = False
        if in_stocks and stripped and not stripped.startswith("symbol"):
            parts = stripped.split("\t")
            if parts and parts[0]:
                symbols.append(parts[0])

    if not symbols:
        result = {"error": "No symbols found in [stocks] section", "updated": 0, "failed": [], "ts": ""}
        _write_status(csv_path, result)
        print(json.dumps(result))
        sys.exit(1)

    # Fetch prices
    prices: dict[str, float] = {}
    failed: list[str] = []
    for sym in symbols:
        price = fetch_price(sym)
        if price is not None:
            prices[sym] = price
            print(f"  {sym}: {price}", file=sys.stderr)
        else:
            failed.append(sym)
            print(f"  {sym}: FAILED", file=sys.stderr)

    # Rewrite [stocks] section in-place; leave everything else unchanged
    new_lines: list[str] = []
    in_stocks = False
    for line in lines:
        stripped = line.strip()
        if stripped == "[stocks]":
            in_stocks = True
            new_lines.append(line)
            continue
        if stripped.startswith("[") and stripped != "[stocks]":
            in_stocks = False
        if in_stocks and stripped and not stripped.startswith("symbol"):
            parts = line.split("\t")
            sym = parts[0] if parts else ""
            if sym in prices:
                parts[2] = f"{prices[sym]:.10f}"
                new_lines.append("\t".join(parts))
                continue
        new_lines.append(line)

    # Atomic write: tmp file then rename
    tmp_path = csv_path + ".tmp"
    try:
        with open(tmp_path, "w", encoding="utf-8", newline="\n") as f:
            f.write("\n".join(new_lines))
        os.replace(tmp_path, csv_path)
    except OSError as e:
        result = {"error": str(e), "updated": 0, "failed": symbols, "ts": ""}
        _write_status(csv_path, result)
        print(json.dumps(result))
        sys.exit(1)

    ts = datetime.datetime.now().strftime("%H:%M")
    result = {"updated": len(prices), "failed": failed, "ts": ts}
    _write_status(csv_path, result)
    print(json.dumps(result))
    sys.exit(0 if not failed or prices else 1)


def _write_status(csv_path: str, result: dict) -> None:
    try:
        with open(csv_path + ".refresh_status", "w", encoding="utf-8") as f:
            json.dump(result, f)
    except OSError:
        pass


if __name__ == "__main__":
    main()
