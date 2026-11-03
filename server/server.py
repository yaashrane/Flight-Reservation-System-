import http.server
import json
import os
import subprocess
import urllib.parse

SERVER_DIR = os.path.dirname(os.path.abspath(__file__))
FRONTEND_DIR = os.path.abspath(os.path.join(SERVER_DIR, '..', 'frontend'))
BACKEND_DIR = os.path.abspath(os.path.join(SERVER_DIR, '..', 'backend'))
BACKEND = os.path.join(BACKEND_DIR, 'main.exe')
PORT = int(os.environ.get('FLIGHTMS_PORT', '8080'))
MAX_REQUEST_SIZE = 1024 * 1024


class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=FRONTEND_DIR, **kwargs)

    def log_message(self, format, *args):
        print(f"[{self.address_string()}] {format % args}")

    def send_json(self, code, body):
        data = body.encode('utf-8')
        self.send_response(code)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.send_header('Content-Length', str(len(data)))
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(data)

    def call_backend(self, payload):
        try:
            result = subprocess.run(
                [BACKEND, json.dumps(payload, ensure_ascii=True)],
                capture_output=True,
                text=True,
                encoding='utf-8',
                timeout=10,
                cwd=BACKEND_DIR
            )
        except FileNotFoundError:
            return 500, {"status": "error", "message": "Backend not compiled. Run compile.bat first."}
        except subprocess.TimeoutExpired:
            return 504, {"status": "error", "message": "Backend request timed out."}

        output = result.stdout.strip()
        if not output:
            message = result.stderr.strip() or "Backend returned an empty response."
            return 500, {"status": "error", "message": message}
        try:
            response = json.loads(output)
        except json.JSONDecodeError:
            return 500, {"status": "error", "message": "Backend returned an invalid response."}
        return 200, response

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        params = dict(urllib.parse.parse_qsl(parsed.query))
        if not params.get('action'):
            return super().do_GET()

        code, response = self.call_backend(params)
        self.send_json(code, json.dumps(response, ensure_ascii=False))

    def do_POST(self):
        try:
            length = int(self.headers.get('Content-Length', '0'))
        except ValueError:
            self.send_json(400, json.dumps({"status": "error", "message": "Invalid content length."}))
            return
        if length <= 0 or length > MAX_REQUEST_SIZE:
            self.send_json(400, json.dumps({"status": "error", "message": "Request body must be between 1 byte and 1 MB."}))
            return
        try:
            payload = json.loads(self.rfile.read(length).decode('utf-8'))
        except (UnicodeDecodeError, json.JSONDecodeError):
            self.send_json(400, json.dumps({"status": "error", "message": "Invalid JSON request body."}))
            return
        if not isinstance(payload, dict) or not isinstance(payload.get('action'), str):
            self.send_json(400, json.dumps({"status": "error", "message": "Request must be a JSON object with an action."}))
            return

        code, response = self.call_backend(payload)
        self.send_json(code, json.dumps(response, ensure_ascii=False))

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()


if __name__ == '__main__':
    os.chdir(SERVER_DIR)
    print(f"Flight Management System running at http://127.0.0.1:{PORT}")
    print(f"Frontend directory: {FRONTEND_DIR}")
    print(f"Backend binary: {BACKEND}")
    http.server.HTTPServer(('127.0.0.1', PORT), Handler).serve_forever()
