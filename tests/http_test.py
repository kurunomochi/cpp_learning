"""Black-box integration tests. Standard library only; never touch a user's DB."""
import concurrent.futures
import json
import os
from pathlib import Path
import socket
import sqlite3
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

TOKEN = "integration-test-token-012345678901234567890123456789"
checks = 0


def require(condition, message):
    global checks
    checks += 1
    if not condition:
        raise RuntimeError(message)


def request(port, method, path, body=None, token=TOKEN, content_type="application/json", raw=None):
    headers = {"Content-Type": content_type}
    if token:
        headers["Authorization"] = "Bearer " + token
    data = raw if raw is not None else (json.dumps(body, ensure_ascii=False).encode("utf-8") if body is not None else None)
    req = urllib.request.Request(f"http://127.0.0.1:{port}{path}", data=data, headers=headers, method=method)
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
    try:
        response = opener.open(req, timeout=10)
    except urllib.error.HTTPError as error:
        response = error
    with response:
        text = response.read().decode("utf-8")
        return response.status, (json.loads(text) if text else None), dict(response.headers)


def launch(binary, directory, port):
    env = os.environ.copy()
    env.update(APP_HOST="127.0.0.1", APP_PORT=str(port), APP_DB=str(directory / "test.db"), APP_TOKEN=TOKEN)
    log = open(directory / "server.log", "ab", buffering=0)
    process = subprocess.Popen([binary], env=env, cwd=directory, stdout=log, stderr=log)
    try:
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError("server exited during startup: " + (directory / "server.log").read_text(errors="replace"))
            try:
                if request(port, "GET", "/healthz")[0] == 200:
                    return process, log
            except (OSError, urllib.error.URLError):
                pass
            time.sleep(0.05)
        raise RuntimeError("server readiness timeout")
    except BaseException:
        stop(process, log)
        raise


def stop(process, log):
    if process.poll() is None:
        process.terminate()  # Windows: abrupt termination; POSIX: SIGTERM.
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=5)
    log.close()


def run(binary):
    with tempfile.TemporaryDirectory(prefix="cpp-tasks-test-") as tmp:
        directory = Path(tmp)
        with socket.socket() as probe:
            probe.bind(("127.0.0.1", 0))
            port = probe.getsockname()[1]
        process, log = launch(binary, directory, port)
        try:
            require(request(port, "GET", "/healthz", token=None)[0] == 200, "health is public")
            require(request(port, "GET", "/api/tasks", token=None)[0] == 401, "API requires token")
            require(request(port, "GET", "/api/tasks", token="wrong")[0] == 401, "wrong token rejected")
            status, first, headers = request(port, "POST", "/api/tasks", {"title": "学习 C++"})
            require(status == 201 and first["title"] == "学习 C++", "create UTF-8 task")
            task_id = first["id"]
            require(headers.get("Location") == f"/api/tasks/{task_id}", "Location header")
            require(bool(headers.get("X-Request-Id")), "request ID header")
            route = f"/api/tasks/{task_id}"
            require(request(port, "GET", route)[1] == first, "read task")
            require(request(port, "PATCH", route, {"completed": True})[1]["completed"] is True, "complete task")
            require(request(port, "PATCH", route, {"completed": False})[1]["completed"] is False, "uncomplete task")
            require(request(port, "PATCH", route, {"title": "renamed"})[1]["title"] == "renamed", "rename task")
            for data in ({}, {"title": ""}, {"title": " "}, {"title": 7}, {"title": "a" * 201}, {"title": "a\nb"}, {"title": "ok", "admin": True}, []):
                require(request(port, "POST", "/api/tasks", data)[0] == 422, f"reject invalid creation {data!r}")
            for data in ({}, {"completed": "false"}, {"completed": None}, {"extra": True}):
                require(request(port, "PATCH", route, data)[0] == 422, f"reject invalid patch {data!r}")
            require(request(port, "POST", "/api/tasks", raw=b"{")[0] == 400, "malformed JSON")
            require(request(port, "POST", "/api/tasks", {"title": "ok"}, content_type="text/plain")[0] == 415, "media type")
            require(request(port, "POST", "/api/tasks", {"title": "x" * 17000})[0] == 413, "body size bound")
            for suffix in ("0", "-1", "abc", "99999999999999999999999999"):
                require(request(port, "GET", "/api/tasks/" + suffix)[0] == 400, "invalid path ID")
            for query in ("limit=0", "limit=101", "offset=-1", "offset=1000001", "limit=1x", "limit=1&limit=2"):
                require(request(port, "GET", "/api/tasks?" + query)[0] == 400, "invalid query")
            require(request(port, "GET", "/missing")[0] == 404, "unknown route")
            require(request(port, "GET", "/api/tasks/999999")[0] == 404, "missing resource")
            require(request(port, "PATCH", "/api/tasks/999999", {"completed": True})[0] == 404, "update missing resource")
            require(request(port, "POST", "/api/tasks", {"title": "a" * 200})[0] == 201, "200-byte title accepted")
            require(request(port, "POST", "/api/tasks", {"title": "学" * 67})[0] == 422, "UTF-8 length uses bytes")
            require(request(port, "POST", "/api/tasks", raw=b'{"title":"\xff"}')[0] == 400, "invalid UTF-8 rejected")
            injection = "x'); DROP TABLE tasks; --"
            require(request(port, "POST", "/api/tasks", {"title": injection})[1]["title"] == injection, "SQL injection stored as text")
            def create(n):
                return request(port, "POST", "/api/tasks", {"title": f"parallel-{n}"})
            with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
                responses = list(pool.map(create, range(24)))
            require(all(result[0] == 201 for result in responses), "concurrent HTTP create")
            require(len({result[1]["id"] for result in responses}) == 24, "unique concurrent IDs")
            page = request(port, "GET", "/api/tasks?limit=2&offset=1")[1]
            require(len(page["items"]) == 2 and page["offset"] == 1, "pagination")
            lock = sqlite3.connect(directory / "test.db")
            try:
                lock.execute("BEGIN IMMEDIATE")
                require(request(port, "POST", "/api/tasks", {"title": "blocked"})[0] == 503, "database contention becomes 503")
            finally:
                lock.rollback()
                lock.close()
            require(request(port, "POST", "/api/tasks", {"title": "after unlock"})[0] == 201, "database recovers after lock release")
        finally:
            stop(process, log)
        process, log = launch(binary, directory, port)
        try:
            require(request(port, "GET", route)[1]["title"] == "renamed", "data survives restart")
            result = request(port, "DELETE", route)
            require(result[0] == 204 and result[1] is None, "delete has empty body")
            require(request(port, "GET", route)[0] == 404, "deleted task gone")
            require(request(port, "DELETE", route)[0] == 404, "repeated delete")
        finally:
            stop(process, log)
        print(f"PASS: {checks} HTTP integration checks (including persistence and concurrency)")


if __name__ == "__main__":
    run(str(Path(sys.argv[1]).resolve()))
