"""Lossy operator publication; file/HTTP I/O never runs on the dispatcher."""

import json
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from auto67_window import LiveStatusWindow


class StatusPublisher:
    def __init__(self, dispatcher, path, html, port=None, window_enabled=False):
        self.dispatcher = dispatcher
        self.path = path
        self.latest = {}
        self.payload = b"{}"
        self.stopped = threading.Event()
        self.server = None
        self.url = None
        self.dropped = 0
        self.window = LiveStatusWindow(path) if window_enabled else None
        if self.window:
            self.window.start()
        if port is not None:
            publisher = self

            class Handler(BaseHTTPRequestHandler):
                def do_GET(self):  # noqa: N802
                    self.connection.settimeout(0.5)
                    body = publisher.payload if self.path == "/api/status" else html.encode()
                    try:
                        self.send_response(200)
                        self.send_header("Content-Type", "application/json" if
                                         self.path == "/api/status" else "text/html; charset=utf-8")
                        self.send_header("Content-Length", str(len(body)))
                        self.end_headers()
                        self.wfile.write(body)
                    except OSError:
                        pass  # Slow/disconnected readers never delay RE work.

                def log_message(self, *_):
                    pass

            self.server = ThreadingHTTPServer(("127.0.0.1", port), Handler)
            self.url = f"http://127.0.0.1:{self.server.server_port}/"
            threading.Thread(target=self.server.serve_forever, daemon=True).start()
        self.thread = threading.Thread(target=self._run, name="auto67-status", daemon=True)
        self.thread.start()

    def publish(self, lua_status):
        # One replaceable status slot, never a queue and never raw event rows.
        self.latest = {key: value for key, value in lua_status.items()
                       if key not in {"events", "discovery"}}

    def _write(self):
        snapshot = self.dispatcher.snapshot(lightweight=True)
        if snapshot is None:
            self.dropped += 1
            return
        lua = self.latest
        payload = {"schema": "oasis.m12.auto67.live-view.v1", "updated_at": time.time(),
                   "frame": lua.get("frame", 0), "lua": lua, "dispatcher": snapshot,
                   "raw_event_backlog": 0, "raw_event_backlog_structure": "NONEXISTENT",
                   "ui_updates_dropped": self.dropped}
        self.payload = json.dumps(payload, separators=(",", ":")).encode()
        temporary = self.path.with_suffix(".view.tmp")
        try:
            temporary.write_bytes(self.payload)
            temporary.replace(self.path)
        except OSError:
            self.dropped += 1

    def _run(self):
        while not self.stopped.is_set():
            self._write()
            self.stopped.wait(0.25)
        self._write()

    def stop(self):
        self.stopped.set()
        self.thread.join(timeout=1)
        if self.server:
            self.server.shutdown()
            self.server.server_close()
        if self.window:
            self.window.stop()
