# IRC Test Harness

`harness.py` is a black-box correctness and robustness test suite for the IRC
server. It checks registration, command replies, channel behavior, fragmented
input, disconnect cleanup, concurrent clients, and slow-reader isolation.

## Quick start

The helper script builds the server, starts it, runs the harness, and stops it:

```bash
./tests/quicktest.sh
```

To use another port or password:

```bash
PORT=6670 PASSWORD=secret ./tests/quicktest.sh
```

## Run against an existing server

Start the server in one terminal:

```bash
./ircserv 6670 pass
```

Run the test suite in another:

```bash
python3 tests/harness.py 127.0.0.1 6670 pass
```

Useful options:

```text
--timeout SECONDS       Increase the response timeout on a slow machine
--stress-clients N      Use 2 to 100 clients in the concurrency scenario
--no-color              Disable colored output
```

For example:

```bash
python3 tests/harness.py 127.0.0.1 6670 pass \
  --stress-clients 100 --timeout 3
```

The harness exits with status `0` when every scenario passes. A failed check or
scenario error produces a nonzero status.

## Reference comparison

The same suite can also be run against an ngIRCd reference server:

```bash
python3 tests/harness.py 127.0.0.1 6670 pass \
  --reference 127.0.0.1:6668 \
  --reference-password pass
```

The reference run is useful for investigating protocol differences. It does not
replace checking the requirements of the ft_irc subject.

## Harness versus load test

Use `harness.py` to check correctness. Use `loadtest.py` to measure capacity,
message throughput, delivery latency, and behavior under sustained pressure.

