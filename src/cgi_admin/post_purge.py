#!/usr/bin/env python3
# Handles DELETE /cgi-bin/post_purge.py (application/x-www-form-urlencoded, field "id")
# from www/admin/index.html's permanent-delete button: removes the matching
# post from www/private/trash.json for good and, if no other post (trashed or
# not) still references it, deletes its uploaded image.
import json
import os
import sys
import urllib.parse

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
WWW_DIR = os.path.join(PROJECT_ROOT, "www")
UPLOAD_DIR = os.path.join(WWW_DIR, "uploads")
POSTS_JSON = os.path.join(WWW_DIR, "data", "posts.json")
TRASH_JSON = os.path.join(WWW_DIR, "private", "trash.json")


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


def remove_image_if_unused(image_url, remaining_trash):
    if not image_url:
        return
    still_used = any(p.get("image") == image_url for p in remaining_trash)
    still_used = still_used or any(p.get("image") == image_url for p in load_json_list(POSTS_JSON))
    if still_used:
        return
    dest_path = os.path.abspath(os.path.join(WWW_DIR, image_url.lstrip("/")))
    if not dest_path.startswith(os.path.abspath(UPLOAD_DIR) + os.sep):
        return
    try:
        os.remove(dest_path)
    except OSError:
        pass


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

    trash = load_json_list(TRASH_JSON)
    target = None
    remaining_trash = []
    for post in trash:
        if target is None and post.get("id") == post_id:
            target = post
            continue
        remaining_trash.append(post)

    if target is None:
        fail("404 Not Found", "Post not found in trash.")

    try:
        save_json_list(TRASH_JSON, remaining_trash)
    except OSError as e:
        fail("500 Internal Server Error", "Failed to save trash: %s" % e)

    remove_image_if_unused(target.get("image"), remaining_trash)

    send("200 OK", json.dumps({"status": "ok", "id": post_id}))


if __name__ == "__main__":
    main()
