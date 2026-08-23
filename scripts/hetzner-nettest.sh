#!/usr/bin/env bash
# Hetzner multi-location download benchmark with loaded-latency (bufferbloat) measurement.
# Downloads a test file from each Hetzner location, one at a time, to a RAM-backed dir, then deletes.
# Appends to a single persistent log; each download is a numbered "Run N" block.\n# Requires Linux, Bash, curl, iproute2, iputils, GNU grep, GNU awk, and coreutils.

set -uo pipefail

SIZE="${SIZE:-100MB}"                    # 100MB | 1GB | 10GB
CYCLES="${CYCLES:-0}"                    # how many full passes; 0 = loop forever
INTERVAL="${INTERVAL:-1}"              # seconds to idle between passes
QUALIFY="${QUALIFY:-1}"                  # 1 = first pass picks the fastest server, then only test that one; 0 = always test all
QUALIFY_SECS="${QUALIFY_SECS:-8}"        # per-server time-boxed probe length during qualifying
WINNER="${WINNER:-}"                     # force a location code (e.g. nbg1) and skip qualifying
RAMDIR="${RAMDIR:-/dev/shm/hetzner-nettest}"
LOG="${LOG:-$HOME/hetzner-nettest.log}"
CSV="${CSV:-$HOME/hetzner-nettest.csv}"
GW="$(ip route show default | awk '/default/{print $3; exit}')"
ISP="${ISP:-${GW:-$REF}}"                # optional ISP gateway; defaults to the router
REF="${REF:-1.1.1.1}"                    # stable off-net reference
PING_N=10                                # unloaded probe count (0.3s apart)

# ---- flags ----
SPEEDONLY=0   # --speedonly : throughput only, skip all latency/bufferbloat probing
NOLOG=0       # --nolog     : print to terminal, write nothing to LOG/CSV
for arg in "$@"; do
  case "$arg" in
    --speedonly) SPEEDONLY=1 ;;
    --nolog)     NOLOG=1 ;;
    -h|--help)
      cat <<EOF
Usage: $(basename "$0") [--speedonly] [--nolog]
  --speedonly  measure download throughput only (no ping / loaded-latency / bufferbloat)
  --nolog      do not append to the log or CSV (terminal output only)
Env knobs: SIZE CYCLES INTERVAL QUALIFY QUALIFY_SECS WINNER ISP REF RAMDIR LOG CSV
EOF
      exit 0 ;;
    *) printf 'Unknown option: %s (try --help)\n' "$arg" >&2; exit 2 ;;
  esac
done

# Each entry: code|city|ping-host|probe-url|main-url
#   probe-url = small file used during qualifying (throughput ranking)
#   main-url  = file downloaded on every ongoing run
# Hetzner files follow $SIZE so `SIZE=1GB` still works; HostDime uses its own catalog.
LOCATIONS=(
  "nbg1|Nuremberg, DE|nbg1-speed.hetzner.com|https://nbg1-speed.hetzner.com/${SIZE}.bin|https://nbg1-speed.hetzner.com/${SIZE}.bin"
  "fsn1|Falkenstein, DE|fsn1-speed.hetzner.com|https://fsn1-speed.hetzner.com/${SIZE}.bin|https://fsn1-speed.hetzner.com/${SIZE}.bin"
  "hel1|Helsinki, FI|hel1-speed.hetzner.com|https://hel1-speed.hetzner.com/${SIZE}.bin|https://hel1-speed.hetzner.com/${SIZE}.bin"
  "ash|Ashburn, VA, US|ash-speed.hetzner.com|https://ash-speed.hetzner.com/${SIZE}.bin|https://ash-speed.hetzner.com/${SIZE}.bin"
  "hil|Hillsboro, OR, US|hil-speed.hetzner.com|https://hil-speed.hetzner.com/${SIZE}.bin|https://hil-speed.hetzner.com/${SIZE}.bin"
  "sin|Singapore, SG|sin-speed.hetzner.com|https://sin-speed.hetzner.com/${SIZE}.bin|https://sin-speed.hetzner.com/${SIZE}.bin"
  "del|Delhi, IN (HostDime)|del.lg.hostdime.com|https://del.lg.hostdime.com/lg/128MB.test|https://del.lg.hostdime.com/lg/1024MB.test"
)

mkdir -p "$RAMDIR" || exit 1

cleanup() {
  # kill any curl/ping still running in this process group, then clear the ramdir
  kill $(jobs -p) 2>/dev/null
  wait 2>/dev/null
  rm -rf "$RAMDIR" 2>/dev/null
}
on_int() {
  printf '\n'
  say "*** Interrupted by user — stopping loop. Partial data is already in $LOG ***"
  cleanup
  exit 130
}
trap on_int INT TERM
trap cleanup EXIT

# say(): always to terminal; append to LOG unless --nolog
if [ "$NOLOG" = "1" ]; then
  say()  { printf '%s\n' "$*"; }
  csv()  { :; }
else
  say()  { printf '%s\n' "$*" | tee -a "$LOG"; }
  csv()  { printf '%s\n' "$*" >> "$CSV"; }
fi

# continue run numbering from whatever is already in the log
RUN=$(grep -oP '^Run \K\d+' "$LOG" 2>/dev/null | sort -n | tail -1)
RUN=$(( ${RUN:-0} + 1 ))

# ping_stats <host> <count> <interval> -> "min avg max mdev loss"
ping_stats() {
  local out loss rtt
  out=$(ping -n -q -c "$2" -i "$3" -W 2 "$1" 2>/dev/null)
  loss=$(printf '%s' "$out" | grep -oP '\d+(?=% packet loss)')
  rtt=$(printf '%s' "$out" | awk -F'= *' '/rtt|round-trip/{print $2}' | tr '/' ' ' | awk '{print $1, $2, $3, $4}')
  [ -z "$rtt" ] && rtt="- - - -"
  printf '%s %s' "$rtt" "${loss:-100}"
}

fmt_mbps() { awk -v b="$1" 'BEGIN{printf "%.2f", b*8/1000000}'; }
fmt_mbs()  { awk -v b="$1" 'BEGIN{printf "%.2f", b/1048576}'; }
ms()       { awk -v s="$1" 'BEGIN{printf "%.1f", s*1000}'; }
delta()    { awk -v a="${1:-0}" -v b="${2:-0}" 'BEGIN{printf "%+.1f", a-b}'; }

SESSION="$(date '+%F %T %Z')"
IFACE="$(ip route get "$REF" 2>/dev/null | awk '{print $5; exit}')"

# CSV header only on first creation
[ "$NOLOG" = 1 ] || [ -s "$CSV" ] || echo "run,session,timestamp,location,city,http_code,bytes,seconds,avg_mbps,peak_mbps,dns_ms,tcp_ms,tls_ms,ttfb_ms,idle_gw_ms,load_gw_ms,idle_isp_ms,load_isp_ms,idle_ref_ms,load_ref_ms,idle_host_ms,load_host_ms,load_isp_max_ms,load_ref_max_ms,isp_loss_pct,ref_loss_pct,remote_ip" > "$CSV"

say ""
say "###############################################################"
say "# Session $SESSION | iface ${IFACE:-?} | gw ${GW:-?} | isp $ISP | ref $REF"
say "# hetzner-size $SIZE (hostdime: 128MB probe / 1024MB run) | cycles $([ "$CYCLES" -eq 0 ] && echo infinite || echo "$CYCLES") | interval ${INTERVAL}s"
say "###############################################################"

# ---------------------------------------------------------------------------
# Qualifying: time-box a download from every server, keep only the fastest.
# Throughput-based (not ping): the winner is the one that actually moves the
# most bytes in QUALIFY_SECS. After this, LOCATIONS holds a single entry.
# ---------------------------------------------------------------------------
qualify() {
  say ""
  say "=== Qualifying (${QUALIFY_SECS}s throughput probe per server) ==="
  local best_entry="" best_city="" best_bps=0
  for entry in "${LOCATIONS[@]}"; do
    local code city host purl murl
    IFS='|' read -r code city host purl murl <<<"$entry"
    # --max-time bounds the probe; speed_download is bytes/s over what was fetched.
    local bps
    bps=$(curl -sS --max-time "$QUALIFY_SECS" -o /dev/null \
           -w '%{speed_download}' "$purl" 2>/dev/null)
    bps=${bps%.*}; [ -z "$bps" ] && bps=0
    say "  $(printf '%-24s' "$city") $(printf '%8s' "$(fmt_mbps "$bps")") Mbps"
    if [ "$bps" -gt "$best_bps" ]; then
      best_bps=$bps; best_entry=$entry; best_city=$city
    fi
  done
  if [ -z "$best_entry" ]; then
    say "  !! No server responded during qualifying — keeping full list."
    return 1
  fi
  LOCATIONS=("$best_entry")
  say "  -> Fastest: $best_city ($(fmt_mbps "$best_bps") Mbps). Ongoing runs will test only this server."
}

if [ -n "$WINNER" ]; then
  # honour a forced winner: find its code, collapse the list to it
  for entry in "${LOCATIONS[@]}"; do
    [ "${entry%%|*}" = "$WINNER" ] && LOCATIONS=("$entry") && break
  done
  say ""
  IFS='|' read -r _ wcity _ <<<"${LOCATIONS[0]}"
  say "=== Winner forced via WINNER=$WINNER -> $wcity ==="
elif [ "$QUALIFY" = "1" ]; then
  qualify
fi

CYCLE=1
while :; do

say ""
say "=== Cycle $CYCLE  ($(date '+%F %T')) ==="
CYCLE_FIRST_RUN=$RUN

for entry in "${LOCATIONS[@]}"; do
  IFS='|' read -r code city host purl url <<<"$entry"   # url = main-url
  target="$RAMDIR/${code}.bin"
  stamp="$(date '+%F %T')"
  ccity="${city//, / - }"   # commas break awk -F, in the summary

  say ""
  say "Run $RUN - $stamp"
  say "  Downloaded from : $city  ($host)"

  # latency vars default empty so the CSV row + set -u stay safe under --speedonly
  igw_avg="" iisp_avg="" iref_avg="" ihst_avg="" ihst_min="" ihst_max=""
  lgw_avg="" lisp_avg="" lref_avg="" lhst_avg="" lisp_max="" lref_max=""
  lisp_loss="" lref_loss=""

  # ---------- baseline (unloaded) ----------
  if [ "$SPEEDONLY" != 1 ]; then
    read -r igw_min igw_avg igw_max igw_mdev igw_loss <<<"$(ping_stats "${GW:-$REF}" $PING_N 0.3)"
    read -r iisp_min iisp_avg iisp_max iisp_mdev iisp_loss <<<"$(ping_stats "$ISP" $PING_N 0.3)"
    read -r iref_min iref_avg iref_max iref_mdev iref_loss <<<"$(ping_stats "$REF" $PING_N 0.3)"
    read -r ihst_min ihst_avg ihst_max ihst_mdev ihst_loss <<<"$(ping_stats "$host" $PING_N 0.3)"
    say "  Unloaded latency: router ${igw_avg} ms | isp-gw ${iisp_avg} ms | ref ${iref_avg} ms | host ${ihst_avg} ms (min ${ihst_min} / max ${ihst_max})"
  fi

  # ---------- loaded download ----------
  rm -f "$target"
  curl -sS --fail-with-body -o "$target" \
    -w '%{http_code} %{size_download} %{time_total} %{speed_download} %{time_namelookup} %{time_connect} %{time_appconnect} %{time_starttransfer} %{remote_ip}\n' \
    "$url" > "$RAMDIR/$code.stats" 2>"$RAMDIR/$code.err" &
  CURL_PID=$!

  if [ "$SPEEDONLY" != 1 ]; then
    ping -n -q -c 10000 -i 0.3 -W 2 "${GW:-$REF}" >"$RAMDIR/$code.pgw"   2>/dev/null & PGW=$!
    ping -n -q -c 10000 -i 0.3 -W 2 "$ISP"        >"$RAMDIR/$code.pisp"  2>/dev/null & PISP=$!
    ping -n -q -c 10000 -i 0.3 -W 2 "$REF"        >"$RAMDIR/$code.pref"  2>/dev/null & PREF=$!
    ping -n -q -c 10000 -i 0.3 -W 2 "$host"       >"$RAMDIR/$code.phost" 2>/dev/null & PHST=$!
  fi

  # 1-second throughput sampling -> peak (+ live progress on the terminal)
  peak=0; prev=0; series=""; elapsed=0
  while kill -0 "$CURL_PID" 2>/dev/null; do
    sleep 1
    elapsed=$((elapsed+1))
    cur=$(stat -c %s "$target" 2>/dev/null || echo 0)
    d=$(( cur - prev )); prev=$cur
    [ "$d" -gt "$peak" ] && peak=$d
    series="$series $(fmt_mbps "$d")"
    # progress goes to the terminal only, never to the log
    [ -t 1 ] && printf '\r    [%s] %3ds  %8s MiB  %7s Mbps now  %7s Mbps peak   ' \
      "$code" "$elapsed" "$(fmt_mbs "$cur")" "$(fmt_mbps "$d")" "$(fmt_mbps "$peak")" >&2
  done
  [ -t 1 ] && printf '\r%*s\r' 78 '' >&2   # wipe the progress line
  wait "$CURL_PID"; curl_rc=$?
  if [ "$SPEEDONLY" != 1 ]; then
    kill -INT "$PGW" "$PISP" "$PREF" "$PHST" 2>/dev/null; wait "$PGW" "$PISP" "$PREF" "$PHST" 2>/dev/null
    parse_p() { awk -F'= *' '/rtt|round-trip/{print $2}' "$1" 2>/dev/null | tr '/' ' ' | awk '{print $1,$2,$3,$4}'; }
    read -r lgw_min  lgw_avg  lgw_max  lgw_mdev  <<<"$(parse_p "$RAMDIR/$code.pgw")"
    read -r lisp_min lisp_avg lisp_max lisp_mdev <<<"$(parse_p "$RAMDIR/$code.pisp")"
    read -r lref_min lref_avg lref_max lref_mdev <<<"$(parse_p "$RAMDIR/$code.pref")"
    read -r lhst_min lhst_avg lhst_max lhst_mdev <<<"$(parse_p "$RAMDIR/$code.phost")"
    lisp_loss=$(grep -oP '\d+(?=% packet loss)' "$RAMDIR/$code.pisp" 2>/dev/null || echo 0)
    lref_loss=$(grep -oP '\d+(?=% packet loss)' "$RAMDIR/$code.pref" 2>/dev/null || echo 0)
  fi

  if [ "$curl_rc" -ne 0 ]; then
    say "  FAILED          : curl rc=$curl_rc — $(head -c 200 "$RAMDIR/$code.err")"
    csv "$RUN,\"$SESSION\",\"$stamp\",$code,\"$ccity\",ERR,0,0,0,0,,,,,${igw_avg},${lgw_avg:-},${iisp_avg},${lisp_avg:-},${iref_avg},${lref_avg:-},${ihst_avg},${lhst_avg:-},${lisp_max:-},${lref_max:-},${lisp_loss:-},${lref_loss:-},"
    rm -f "$target" "$RAMDIR/$code".{stats,err,pgw,pisp,pref,phost}
    RUN=$((RUN+1)); continue
  fi

  read -r hcode bytes secs bps t_dns t_tcp t_tls t_ttfb rip < "$RAMDIR/$code.stats"
  avg_mbps=$(fmt_mbps "$bps"); peak_mbps=$(fmt_mbps "$peak")

  if [ "$SPEEDONLY" != 1 ]; then
    say "  Loaded latency  : router ${lgw_avg} ms | isp-gw ${lisp_avg} ms (max ${lisp_max}) | ref ${lref_avg} ms (max ${lref_max}) | host ${lhst_avg} ms"
    say "  Bufferbloat     : router $(delta "$lgw_avg" "$igw_avg") ms | isp-gw $(delta "$lisp_avg" "$iisp_avg") ms | ref $(delta "$lref_avg" "$iref_avg") ms"
    say "  Packet loss     : isp-gw ${lisp_loss}% | ref ${lref_loss}%"
  fi
  say "  Connect phases  : dns $(ms "$t_dns") ms | tcp $(ms "$t_tcp") ms | tls $(ms "$t_tls") ms | ttfb $(ms "$t_ttfb") ms | peer $rip"
  say "  Downloaded      : $(fmt_mbs "$bytes") MiB (HTTP $hcode)"
  say "  Time to download: ${secs} s"
  say "  Transfer rate   : ${avg_mbps} Mbps avg ($(fmt_mbs "$bps") MiB/s)"
  say "  Peak rate       : ${peak_mbps} Mbps"
  say "  Per-second Mbps :${series}"

  csv "$RUN,\"$SESSION\",\"$stamp\",$code,\"$ccity\",$hcode,$bytes,$secs,$avg_mbps,$peak_mbps,$(ms "$t_dns"),$(ms "$t_tcp"),$(ms "$t_tls"),$(ms "$t_ttfb"),${igw_avg},${lgw_avg},${iisp_avg},${lisp_avg},${iref_avg},${lref_avg},${ihst_avg},${lhst_avg},${lisp_max},${lref_max},${lisp_loss},${lref_loss},$rip"

  rm -f "$target" "$RAMDIR/$code".{stats,err,pgw,pisp,pref,phost}
  RUN=$((RUN+1))
  sleep 3   # let queues drain before the next run
done

say ""
say "--- Cycle $CYCLE summary (by avg throughput) ---"
if [ "$NOLOG" = 1 ]; then
  say "  (--nolog: per-run results above; nothing written to log/CSV)"
elif [ "$SPEEDONLY" = 1 ]; then
  say "$(awk -F, -v s="\"$SESSION\"" -v c="$CYCLE_FIRST_RUN" 'NR>1 && $2==s && $1>=c && $6!="ERR" {gsub(/"/,"",$5); printf "  %-22s %8s Mbps avg | %8s peak\n", $5, $9, $10}' "$CSV" | sort -k2 -gr)"
  say "Log: $LOG   CSV: $CSV"
else
  say "$(awk -F, -v s="\"$SESSION\"" -v c="$CYCLE_FIRST_RUN" 'NR>1 && $2==s && $1>=c && $6!="ERR" {gsub(/"/,"",$5); printf "  %-22s %8s Mbps avg | %8s peak | ttfb %6s ms | bloat: router %+.1f / isp %+.1f / ref %+.1f ms\n", $5, $9, $10, $14, $16-$15, $18-$17, $20-$19}' "$CSV" | sort -k2 -gr)"
  say "Log: $LOG   CSV: $CSV"
fi

  CYCLE=$((CYCLE+1))
  [ "$CYCLES" -ne 0 ] && [ "$CYCLE" -gt "$CYCLES" ] && break
  say ""
  say "Sleeping ${INTERVAL}s until next cycle (Ctrl-C to stop)..."
  sleep "$INTERVAL"
done
