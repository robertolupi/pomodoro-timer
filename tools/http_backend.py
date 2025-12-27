#!/usr/bin/env python3
import json
import os
import re
from http.server import BaseHTTPRequestHandler, HTTPServer


class PomodoroHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        match = re.match(r"^/pomodoros/(\d+)/transitions/?$", self.path)
        if not match:
            self.send_error(404, "Not Found")
            return

        content_length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(content_length)
        try:
            payload = json.loads(body.decode("utf-8"))
        except json.JSONDecodeError:
            self.send_error(400, "Invalid JSON")
            return

        start_time = match.group(1)
        event_time = payload.get("event_time")
        if isinstance(event_time, str) and event_time.isdigit():
            event_time = int(event_time)
        if not isinstance(event_time, int):
            self.send_error(400, "Missing or invalid event_time")
            return

        os.makedirs("received", exist_ok=True)
        filename = f"{start_time}-{event_time}.json"
        with open(os.path.join("received", filename), "w", encoding="utf-8") as handle:
            json.dump(payload, handle, indent=2, sort_keys=True)

        self.send_response(201)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(b'{"status":"ok"}')

    def log_message(self, format, *args):
        return


def main():
    host = "0.0.0.0"
    port = 8080
    server = HTTPServer((host, port), PomodoroHandler)
    print(f"Listening on http://{host}:{port}")
    server.serve_forever()


if __name__ == "__main__":
    main()
