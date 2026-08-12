#!/usr/bin/env python3
# Handles DELETE /cgi-bin/post_delete.py (application/x-www-form-urlencoded, field "id")
# from www/index.html's delete button: moves the matching post from
# www/data/posts.json into www/data/trash.json (soft delete) so the admin
# board (see src/cgi_admin/post_restore.py) can bring it back later. The
# uploaded image is kept on disk until the post is purged from the trash.
import json
import os
import sys
import time
import urllib.parse

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
WWW_DIR = os.path.join(PROJECT_ROOT, "www")
POSTS_JSON = os.path.join(WWW_DIR, "data", "posts.json")
TRASH_JSON = os.path.join(WWW_DIR, "data", "trash.json")


def send(status_line, body, content_type="application/json"):
    body_bytes = body.encode("utf-8")
    headers = "Status: %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n" % (
        status_line, content_type, len(body_bytes)
    )
    sys.stdout.write(headers)
    sys.stdout.flush()
    sys.stdout.buffer.write(body_bytes)
    sys.exit(0)


def fail(status_line, message):
    send(status_line, json.dumps({"status": "error", "message": message}))


def load_json_list(path):
    if not os.path.exists(path):
        return []
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
        return data if isinstance(data, list) else []
    except (ValueError, OSError):
        return []


def save_json_list(path, items):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    tmp_path = path + ".tmp"
    with open(tmp_path, "w", encoding="utf-8") as f:
        json.dump(items, f, ensure_ascii=False, indent=2)
    os.replace(tmp_path, path)


def main():
    if os.environ.get("REQUEST_METHOD", "") != "DELETE":
        fail("405 Method Not Allowed", "DELETE only.")

    try:
        length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
    except ValueError:
        length = 0
    body = sys.stdin.buffer.read(length) if length > 0 else b""
    fields = urllib.parse.parse_qs(body.decode("utf-8", "replace"))
    post_id = (fields.get("id") or [""])[0].strip()
    if not post_id:
        fail("400 Bad Request", "id is required.")

    posts = load_json_list(POSTS_JSON)
    target = None
    remaining = []
    for post in posts:
        if target is None and post.get("id") == post_id:
            target = post
            continue
        remaining.append(post)

    if target is None:
        fail("404 Not Found", "Post not found.")

    target["deleted_at"] = time.strftime("%Y-%m-%d %H:%M:%S")
    trash = load_json_list(TRASH_JSON)
    trash.insert(0, target)

    try:
        save_json_list(TRASH_JSON, trash)
        save_json_list(POSTS_JSON, remaining)
    except OSError as e:
        fail("500 Internal Server Error", "Failed to save posts: %s" % e)

    send("200 OK", json.dumps({"status": "ok", "id": post_id}))


if __name__ == "__main__":
    main()
