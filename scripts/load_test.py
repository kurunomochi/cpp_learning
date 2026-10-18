"""Small learning load generator; not a production benchmark framework."""
import argparse
import concurrent.futures
import math
import os
import time
import urllib.request

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--url", default="http://127.0.0.1:8080/healthz")
parser.add_argument("--requests", type=int, default=200)
parser.add_argument("--workers", type=int, default=8)
args = parser.parse_args()
if not 1 <= args.requests <= 10000 or not 1 <= args.workers <= 64:
    parser.error("requests must be 1..10000; workers must be 1..64")
token = os.environ.get("APP_TOKEN", "")


def run_one(_):
    headers = {"Authorization": "Bearer " + token} if token else {}
    req = urllib.request.Request(args.url, headers=headers)
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
    start = time.perf_counter()
    try:
        with opener.open(req, timeout=5) as response:
            response.read()
            ok = response.status == 200
    except Exception:
        ok = False
    return ok, (time.perf_counter() - start) * 1000


start = time.perf_counter()
with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
    results = list(pool.map(run_one, range(args.requests)))
elapsed = time.perf_counter() - start
latencies = sorted(ms for ok, ms in results if ok)
print(f"requests={len(results)} errors={len(results)-len(latencies)} elapsed={elapsed:.3f}s")
print(f"successful_requests_per_second={len(latencies)/elapsed:.1f}")
if latencies:
    for p in (0.50, 0.95, 0.99):
        print(f"p{int(p*100)}={latencies[math.ceil(p*len(latencies))-1]:.2f}ms (successful requests)")
