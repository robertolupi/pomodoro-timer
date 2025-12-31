#!/usr/bin/env python3
import json
import os
import re
import sqlite3
from datetime import datetime
from http.server import BaseHTTPRequestHandler, HTTPServer


def init_database():
    """Initialize the SQLite database and create tables if they don't exist."""
    conn = sqlite3.connect('pomodoros.db')
    cursor = conn.cursor()
    
    # Create pomodoros table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS pomodoros (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            start_time INTEGER NOT NULL,
            end_time INTEGER,
            work_flavor TEXT,
            work_duration INTEGER,
            break_duration INTEGER,
            cancelled BOOLEAN DEFAULT FALSE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    ''')
    
    # Create transitions table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS transitions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            pomodoro_id INTEGER,
            transition_type TEXT NOT NULL,
            event_time INTEGER NOT NULL,
            payload_json TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (pomodoro_id) REFERENCES pomodoros(id)
        )
    ''')
    
    conn.commit()
    conn.close()


def save_to_database(start_time, payload):
    """Save pomodoro data to SQLite database."""
    conn = sqlite3.connect('pomodoros.db')
    cursor = conn.cursor()
    
    transition_type = payload.get('transition')
    event_time = payload.get('event_time')
    
    # Find or create pomodoro record
    cursor.execute('''
        SELECT id FROM pomodoros WHERE start_time = ?
    ''', (start_time,))
    pomodoro_record = cursor.fetchone()
    
    if transition_type == 'idle_to_work':
        # Start of a new pomodoro
        work_flavor = payload.get('work_flavor', "0")
        if work_flavor is not None and not isinstance(work_flavor, str):
            work_flavor = str(work_flavor)
        if pomodoro_record is None:
            cursor.execute('''
                INSERT INTO pomodoros (start_time, work_flavor)
                VALUES (?, ?)
            ''', (start_time, work_flavor))
            pomodoro_id = cursor.lastrowid
        else:
            pomodoro_id = pomodoro_record[0]
            cursor.execute('''
                UPDATE pomodoros SET work_flavor = ? WHERE id = ?
            ''', (work_flavor, pomodoro_id))
    
    elif transition_type in ['work_to_break', 'work_to_idle']:
        # End of work period
        if pomodoro_record is None:
            # Create record if it doesn't exist (shouldn't happen normally)
            cursor.execute('''
                INSERT INTO pomodoros (start_time, end_time)
                VALUES (?, ?)
            ''', (start_time, event_time))
            pomodoro_id = cursor.lastrowid
        else:
            pomodoro_id = pomodoro_record[0]
            cursor.execute('''
                UPDATE pomodoros SET end_time = ? WHERE id = ?
            ''', (event_time, pomodoro_id))
        
        # Update additional fields based on transition type
        if transition_type == 'work_to_break':
            work_duration = payload.get('work_duration')
            cursor.execute('''
                UPDATE pomodoros SET work_duration = ? WHERE id = ?
            ''', (work_duration, pomodoro_id))
        else:  # work_to_idle (cancelled)
            cancelled_duration = payload.get('cancelled_work_duration')
            cursor.execute('''
                UPDATE pomodoros SET work_duration = ?, cancelled = TRUE WHERE id = ?
            ''', (cancelled_duration, pomodoro_id))
    
    elif transition_type == 'break_to_idle':
        # End of break period
        if pomodoro_record:
            pomodoro_id = pomodoro_record[0]
            break_duration = payload.get('break_duration')
            cursor.execute('''
                UPDATE pomodoros SET break_duration = ? WHERE id = ?
            ''', (break_duration, pomodoro_id))
        else:
            pomodoro_id = None
    else:
        pomodoro_id = pomodoro_record[0] if pomodoro_record else None
    
    # Always save the transition
    if pomodoro_id:
        cursor.execute('''
            INSERT INTO transitions (pomodoro_id, transition_type, event_time, payload_json)
            VALUES (?, ?, ?, ?)
        ''', (pomodoro_id, transition_type, event_time, json.dumps(payload)))
    else:
        cursor.execute('''
            INSERT INTO transitions (transition_type, event_time, payload_json)
            VALUES (?, ?, ?)
        ''', (transition_type, event_time, json.dumps(payload)))
    
    conn.commit()
    conn.close()


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

        # Save to JSON file (original functionality)
        os.makedirs("received", exist_ok=True)
        filename = f"{start_time}-{event_time}.json"
        with open(os.path.join("received", filename), "w", encoding="utf-8") as handle:
            json.dump(payload, handle, indent=2, sort_keys=True)

        # Save to SQLite database (new functionality)
        try:
            save_to_database(int(start_time), payload)
        except Exception as e:
            print(f"Error saving to database: {e}")
            # Don't fail the request if database fails

        self.send_response(201)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(b'{"status":"ok"}')

    def log_message(self, format, *args):
        return


def main():
    # Initialize database
    init_database()
    
    host = "0.0.0.0"
    port = 8080
    server = HTTPServer((host, port), PomodoroHandler)
    print(f"Listening on http://{host}:{port}")
    print("Database: pomodoros.db")
    print("JSON logs: received/")
    server.serve_forever()


if __name__ == "__main__":
    main()
