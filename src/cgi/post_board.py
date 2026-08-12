#!/usr/bin/env python3
# Handles the multipart/form-data POST from www/public/upload.html (title, content, image),
# stores the image under www/uploads/ and appends the post to www/data/posts.json,
# then sends a client redirect (RFC3875 6.2.3) back to the board.
import json
import os
import re
import sys
import time
import uuid

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
WWW_DIR = os.path.join(PROJECT_ROOT, "www")
UPLOAD_DIR = os.path.join(WWW_DIR, "uploads")
POSTS_JSON = os.path.join(WWW_DIR, "data", "posts.json")


def send(status_line, body, content_type="text/html; charset=utf-8"):
    body_bytes = body.encode("utf-8")
    headers = "Status: %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n" % (
        status_line, content_type, len(body_bytes)
    )
    sys.stdout.write(headers)
    sys.stdout.flush()
    sys.stdout.buffer.write(body_bytes)
    sys.exit(0)


def bad_request(message):
    send("400 Bad Request", "<h1>400 Bad Request</h1><p>%s</p>" % message)


def server_error(message):
    send("500 Internal Server Error", "<h1>500 Internal Server Error</h1><p>%s</p>" % message)


def escape_html(text):
    return (
        text.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
        .replace("'", "&#39;")
    )


def parse_content_disposition(header_value):
    params = {}
    for part in header_value.split(";")[1:]:
        part = part.strip()
        m = re.match(r'([^=]+)="(.*)"$', part)
        if m:
            params[m.group(1)] = m.group(2)
    return params


def parse_multipart(body, boundary):
    delimiter = ("--" + boundary).encode("utf-8")
    fields = {}
    files = {}

    for raw_part in body.split(delimiter):
        if raw_part in (b"", b"--\r\n", b"--"):
            continue
        part = raw_part[2:] if raw_part.startswith(b"\r\n") else raw_part
        if part.endswith(b"\r\n"):
            part = part[:-2]

        header_end = part.find(b"\r\n\r\n")
        if header_end == -1:
            continue
        header_block = part[:header_end].decode("utf-8", "replace")
        content = part[header_end + 4:]

        disposition = None
        for line in header_block.split("\r\n"):
            if line.lower().startswith("content-disposition:"):
                disposition = line.split(":", 1)[1].strip()
                break
        if disposition is None:
            continue

        params = parse_content_disposition(disposition)
        name = params.get("name")
        if name is None:
            continue

        if "filename" in params:
            files[name] = {"filename": params["filename"], "content": content}
        else:
            fields[name] = content.decode("utf-8", "replace")

    return fields, files


def sanitize_filename(filename):
    base = os.path.basename(filename.replace("\\", "/"))
    base = re.sub(r"[^A-Za-z0-9._-]", "_", base)
    return base or "upload"


def save_image(file_field):
    original = sanitize_filename(file_field["filename"])
    unique_name = "%d_%s_%s" % (int(time.time()), uuid.uuid4().hex[:8], original)
    os.makedirs(UPLOAD_DIR, exist_ok=True)
    dest_path = os.path.join(UPLOAD_DIR, unique_name)
    with open(dest_path, "wb") as f:
        f.write(file_field["content"])
    return "/uploads/" + unique_name


def append_post(post):
    posts = []
    if os.path.exists(POSTS_JSON):
        try:
            with open(POSTS_JSON, "r", encoding="utf-8") as f:
                data = json.load(f)
            if isinstance(data, list):
                posts = data
        except (ValueError, OSError):
            posts = []

    posts.insert(0, post)

    os.makedirs(os.path.dirname(POSTS_JSON), exist_ok=True)
    tmp_path = POSTS_JSON + ".tmp"
    with open(tmp_path, "w", encoding="utf-8") as f:
        json.dump(posts, f, ensure_ascii=False, indent=2)
    os.replace(tmp_path, POSTS_JSON)


def main():
    if os.environ.get("REQUEST_METHOD", "") != "POST":
        bad_request("POST only.")

    content_type = os.environ.get("CONTENT_TYPE", "")
    boundary_match = re.search(r'boundary=(?:"([^"]+)"|([^;]+))', content_type)
    if "multipart/form-data" not in content_type or not boundary_match:
        bad_request("Expected multipart/form-data.")
    boundary = boundary_match.group(1) or boundary_match.group(2).strip()

    try:
        length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
    except ValueError:
        length = 0
    if length <= 0:
        bad_request("Empty body.")

    body = sys.stdin.buffer.read(length)
    fields, files = parse_multipart(body, boundary)

    title = fields.get("title", "").strip()
    if not title:
        bad_request("Title is required.")
    content = fields.get("content", "").strip()

    image_url = None
    image_file = files.get("image")
    if image_file and image_file["filename"]:
        try:
            image_url = save_image(image_file)
        except OSError as e:
            server_error("Failed to save image: %s" % e)

    post = {
        "id": uuid.uuid4().hex,
        "title": escape_html(title),
        "content": escape_html(content),
        "image": image_url,
        "date": time.strftime("%Y-%m-%d %H:%M:%S"),
    }

    try:
        append_post(post)
    except OSError as e:
        server_error("Failed to save post: %s" % e)

    host = os.environ.get("SERVER_NAME", "localhost")
    port = os.environ.get("SERVER_PORT", "")
    location = "http://%s:%s/index.html" % (host, port) if port else "http://%s/index.html" % host
    body_text = "redirecting...\n"
    headers = "Status: 302 Found\r\nLocation: %s\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n" % (
        location, len(body_text)
    )
    sys.stdout.write(headers)
    sys.stdout.flush()
    sys.stdout.buffer.write(body_text.encode("utf-8"))


if __name__ == "__main__":
    main()
