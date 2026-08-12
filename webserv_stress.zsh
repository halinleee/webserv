#!/usr/bin/env zsh
# webserv_stress.zsh — fire N concurrent clients at one HTTP request and
# report status-code breakdown + timing. Uses curl (already required by the
# rest of tests/script), fanned out with xargs -P — no siege/ab dependency.
#
# Usage:
#   ./webserv_stress.zsh URL CONCURRENCY [-n REQUESTS] [-X METHOD] [-d BODY] [-H HEADER]...
#
#   URL          request target, e.g. localhost:8080/ (http:// added if missing)
#   CONCURRENCY  number of concurrent clients
#   -n  total requests to send   (default = CONCURRENCY, i.e. one wave)
#   -X  HTTP method              (default GET)
#   -d  request body (implies -X POST unless -X given)
#   -H  extra header, repeatable
#
# Examples:
#   ./webserv_stress.zsh localhost:8080/ 50
#   ./webserv_stress.zsh localhost:8080/ 20 -n 500
#   ./webserv_stress.zsh localhost:8080/cgi-bin/post_board.py 20 -X POST -d '{"a":1}' -H 'Content-Type: application/json'

emulate -L zsh
set -u

if (( $# < 2 )); then
  print -u2 -- "usage: $0 URL CONCURRENCY [-n REQUESTS] [-X METHOD] [-d BODY] [-H HEADER]..."
  exit 2
fi

URL=$1; shift
CONCURRENCY=$1; shift
[[ $URL != http://* && $URL != https://* ]] && URL="http://${URL}"

REQUESTS=0
METHOD=GET
BODY=""
typeset -a HEADERS
HEADERS=()

while getopts "n:X:d:H:" opt; do
  case $opt in
    n) REQUESTS=$OPTARG ;;
    X) METHOD=$OPTARG ;;
    d) BODY=$OPTARG; [[ $METHOD == GET ]] && METHOD=POST ;;
    H) HEADERS+=("$OPTARG") ;;
    *) exit 2 ;;
  esac
done

(( REQUESTS == 0 )) && REQUESTS=$CONCURRENCY

typeset -a CURL_ARGS
CURL_ARGS=(-s -o /dev/null -w '%{http_code} %{time_total}\n' -X "$METHOD")
[[ -n $BODY ]] && CURL_ARGS+=(-d "$BODY")
for h in $HEADERS; do CURL_ARGS+=(-H "$h"); done

print -- "target:      $URL"
print -- "method:      $METHOD"
print -- "concurrency: $CONCURRENCY"
print -- "requests:    $REQUESTS"
print -- ""

RESULTS=$(mktemp -t webserv_stress.XXXXXX)
seq "$REQUESTS" | xargs -P "$CONCURRENCY" -I{} curl "${CURL_ARGS[@]}" "$URL" >> "$RESULTS" 2>/dev/null

print -- "── status codes ──"
awk '{print $1}' "$RESULTS" | sort | uniq -c | sort -rn

print -- ""
print -- "── latency (s) ──"
awk '{sum+=$2; n++; if(min==""||$2<min)min=$2; if($2>max)max=$2}
     END{ if(n) printf "n=%d  min=%.3f  avg=%.3f  max=%.3f\n", n, min, sum/n, max
          else print "no responses received" }' "$RESULTS"

rm -f "$RESULTS"
