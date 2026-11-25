#!/usr/bin/env bash

LOG_DIR="$HOME/.system_monitor"; LOG_FILE="$LOG_DIR/activity.log"
DEFAULT_REPORT_DIR="$HOME/sysmon_reports"
CPU_T=80; MEM_T=80; DISK_T=80                       # default thresholds (%)
PROTECTED=(systemd init sshd kthreadd dbus-daemon login agetty cron crond rsyslogd
           NetworkManager systemd-journal systemd-logind systemd-udevd udevd Xorg
           Xwayland gdm gdm3 lightdm containerd dockerd)
PSF="pid,ppid,user,%cpu,%mem,stat,etime,comm"       # common ps columns

# ---------- helpers ----------
log()  { printf '%s [%s] %s\n' "$(date '+%F %T')" "$1" "$2" >> "$LOG_FILE" 2>/dev/null; }
err()  { echo "[ERROR] $*" >&2; }
warn() { echo "[WARNING] $*"; log WARNING "$*"; }
confirm() { local a; read -rp "$1 [y/N]: " a; [[ $a =~ ^[Yy]([Ee][Ss])?$ ]]; }
over() { awk -v v="$1" -v t="$2" 'BEGIN{exit !(v>t)}'; }          # v > t (decimals ok)

# read_threshold LABEL CURRENT -> prints a valid 1-100 value
read_threshold() {
    local i
    while read -rp "Enter $1 threshold % (1-100) [Enter = keep $2]: " i; do
        [[ -z $i ]] && { echo "$2"; return; }
        [[ $i =~ ^[0-9]+$ ]] && ((10#$i >= 1 && 10#$i <= 100)) && { echo $((10#$i)); return; }
        err "'$i' is not valid. Enter a whole number from 1 to 100." >&2
    done
    echo "$2"
}

# ---------- data sections (used by menu AND report) ----------
uptime_str() { uptime -p 2>/dev/null || awk '{printf "up %d min\n",$1/60}' /proc/uptime; }

sec_sysinfo() {
    echo "Hostname       : $(hostname 2>/dev/null || uname -n)"
    echo "Operating Sys. : $( . /etc/os-release 2>/dev/null && echo "$PRETTY_NAME" || uname -s)"
    echo "Kernel version : $(uname -r)"
    echo "Uptime         : $(uptime_str)"
    echo "Date and time  : $(date '+%F %T %Z')"
}

cpu_usage() {                                       # overall % sampled over 1 second
    [[ -r /proc/stat ]] || return 1
    local a b t1=0 t2=0 i dt di
    read -ra a < /proc/stat; sleep 1; read -ra b < /proc/stat
    for i in {1..8}; do t1=$((t1 + ${a[i]:-0})); t2=$((t2 + ${b[i]:-0})); done
    dt=$((t2 - t1)); di=$((b[4] + ${b[5]:-0} - a[4] - ${a[5]:-0}))
    echo $(( dt > 0 ? 100 * (dt - di) / dt : 0 ))
}

sec_cpu() {
    local u heavy
    u=$(cpu_usage) || { err "CPU statistics unavailable (/proc/stat)."; return 1; }
    echo "Overall CPU usage : ${u}%  (threshold ${CPU_T}%)"
    [[ -r /proc/loadavg ]] && echo "Load average      : $(cut -d' ' -f1-3 /proc/loadavg)"
    echo; echo "Top 5 CPU-consuming processes:"
    ps -eo pid,user,%cpu,%mem,comm --sort=-%cpu | head -n 6; echo
    heavy=$(ps -eo pid=,user=,%cpu=,%mem=,comm= | awk -v t="$CPU_T" '$3+0 >= t')
    [[ -n $heavy ]] && { warn "Processes at/above ${CPU_T}% CPU:"; echo "$heavy"; }
    if ((u > CPU_T)); then warn "Overall CPU usage ${u}% exceeds ${CPU_T}%."
    else echo "CPU usage is within the threshold."; fi
}

sec_mem() {
    local total avail used pct
    [[ -r /proc/meminfo ]] || { err "Memory info unavailable (/proc/meminfo)."; return 1; }
    read -r total avail < <(awk '/^MemTotal:/{t=$2} /^MemAvailable:/{a=$2}
        /^(MemFree|Buffers|Cached):/{f+=$2} END{print int(t/1024), int((a?a:f)/1024)}' /proc/meminfo)
    ((total > 0)) || { err "Could not read total memory."; return 1; }
    used=$((total - avail)); pct=$(awk -v u=$used -v t=$total 'BEGIN{printf "%.1f", u*100/t}')
    printf 'Total memory     : %s MiB\nUsed memory      : %s MiB\nAvailable memory : %s MiB\n' "$total" "$used" "$avail"
    echo "Utilization      : ${pct}%  (threshold ${MEM_T}%)"
    if over "$pct" "$MEM_T"; then warn "Memory utilization ${pct}% exceeds ${MEM_T}%."
    else echo "Memory utilization is within the threshold."; fi
}

sec_disk() {
    local out fs size used avail pct mnt hit=0
    out=$(df -hP -x tmpfs -x devtmpfs -x squashfs -x overlay 2>/dev/null || df -hP 2>/dev/null)
    [[ -n $out ]] || { err "Unable to read disk info with 'df'."; return 1; }
    printf '%-28s %7s %7s %7s %5s  %s\n' Filesystem Size Used Avail Use% "Mounted on"
    while read -r fs size used avail pct mnt; do
        pct=${pct%\%}; [[ $pct =~ ^[0-9]+$ ]] || continue
        printf '%-28s %7s %7s %7s %4s%%  %s\n' "$fs" "$size" "$used" "$avail" "$pct" "$mnt"
        ((pct >= DISK_T)) && { hit=1; warn "$mnt ($fs) is ${pct}% full (threshold ${DISK_T}%)."; }
    done < <(tail -n +2 <<< "$out")
    ((hit)) || echo "No filesystem has reached the ${DISK_T}% threshold."
}

# ---------- menu actions ----------
monitor() {                                         # monitor LABEL THRESHOLD_VAR SECTION_FN
    echo "=== $1 Utilization ==="
    printf -v "$2" '%s' "$(read_threshold "$1" "${!2}")"
    "$3"; log INFO "Monitored $1 utilization (threshold ${!2}%)"
}

search_process() {
    local t m
    read -rp "Enter process name or PID: " t
    [[ -n $t ]] || { err "Search term cannot be empty."; return; }
    if [[ $t =~ ^[0-9]+$ ]]; then
        ps -p "$t" -o "$PSF" 2>/dev/null || err "No process with PID $t exists."
    else
        m=$(ps -eo "${PSF// /}" --no-headers | awk -v p="$t" 'index(tolower($8),tolower(p))')
        [[ -n $m ]] && { echo "  PID  PPID USER      %CPU %MEM STAT  ELAPSED COMMAND"; echo "$m"; } \
                    || err "No running process matches '$t'."
    fi
    log INFO "Searched for process '$t'"
}

view_processes() {
    local c
    while echo -e "\n=== Running Processes ===\n1) Top 20 by CPU\n2) Top 20 by memory\n3) All (paged)\n4) Search by name/PID\n5) Back"; do
        read -rp "Choose [1-5]: " c || return
        case $c in
            1) ps -eo pid,user,%cpu,%mem,stat,comm --sort=-%cpu | head -n 21; log INFO "Viewed top processes by CPU" ;;
            2) ps -eo pid,user,%cpu,%mem,stat,comm --sort=-%mem | head -n 21; log INFO "Viewed top processes by memory" ;;
            3) ps -eo pid,user,%cpu,%mem,stat,comm | ${PAGER:-more}; log INFO "Viewed all processes" ;;
            4) search_process ;;
            5) return ;;
            *) err "Invalid choice '$c'. Enter 1-5." ;;
        esac
    done
}

terminate_process() {
    local pid comm uid ppid n why=""
    echo "=== Terminate a Process ==="
    read -rp "Enter the PID to terminate: " pid
    [[ $pid =~ ^[0-9]+$ ]] || { err "'$pid' is not a valid PID."; return; }
    pid=$((10#$pid))
    ps -p "$pid" >/dev/null 2>&1 || { err "Process $pid does not exist."; log ERROR "Terminate failed: PID $pid not found"; return; }
    read -r uid ppid comm < <(ps -o uid=,ppid=,comm= -p "$pid")

    # safety check 1: protected processes
    if   ((pid <= 2));               then why="core system process"
    elif ((pid == $$ || pid == PPID)); then why="this script or its parent shell"
    elif ((ppid == 2));              then why="kernel thread"
    else for n in "${PROTECTED[@]}"; do [[ $comm == "$n" ]] && why="protected system process '$comm'"; done
    fi
    [[ -n $why ]] && { err "Refusing to terminate PID $pid: $why."; log WARNING "Blocked termination of PID $pid ($comm)"; return; }
    # safety check 2: permissions
    ((EUID != 0 && uid != EUID)) && { err "Permission denied: PID $pid belongs to another user."; log WARNING "Permission denied for PID $pid ($comm)"; return; }

    ps -p "$pid" -o "$PSF"
    confirm "Send SIGTERM to PID $pid ($comm)?" || { echo "Cancelled."; log INFO "Termination of PID $pid cancelled"; return; }
    kill -TERM "$pid" 2>/dev/null || { err "Failed to signal PID $pid."; log ERROR "kill failed for PID $pid"; return; }
    sleep 2
    if ! ps -p "$pid" >/dev/null 2>&1; then echo "Process $pid ($comm) terminated."; log INFO "Terminated PID $pid ($comm)"
    else
        warn "PID $pid still running after SIGTERM."
        confirm "Force kill with SIGKILL?" && kill -KILL "$pid" 2>/dev/null \
            && { echo "Process $pid force-killed."; log INFO "Force-killed PID $pid ($comm)"; }
    fi
}

generate_report() {
    local dir file
    echo "=== Generate System Health Report ==="
    read -rp "Directory to save report [Enter = $DEFAULT_REPORT_DIR]: " dir
    dir="${dir:-$DEFAULT_REPORT_DIR}"; dir="${dir/#\~/$HOME}"
    if [[ ! -d $dir ]]; then
        confirm "'$dir' does not exist. Create it?" || { echo "Cancelled."; return; }
        mkdir -p "$dir" 2>/dev/null || { err "Cannot create '$dir'."; log ERROR "Report failed: cannot create $dir"; return; }
    fi
    [[ -w $dir ]] || { err "'$dir' is not writable."; log ERROR "Report failed: $dir not writable"; return; }
    file="${dir%/}/health_report_$(date +%Y%m%d_%H%M%S).txt"
    echo "Collecting data..."
    { echo "SYSTEM HEALTH REPORT - generated $(date '+%F %T %Z')"
      echo; echo "--- System Information ---"; sec_sysinfo
      echo; echo "--- CPU ---";    sec_cpu
      echo; echo "--- Memory ---"; sec_mem
      echo; echo "--- Disk ---";   sec_disk
      echo; echo "--- Uptime ---"; uptime_str
    } > "$file" 2>/dev/null || { err "Could not write '$file'."; log ERROR "Report failed: $file"; return; }
    echo "Report saved: $file"; log INFO "Generated health report: $file"
}

view_log() {
    local n
    [[ -s $LOG_FILE ]] || { echo "The activity log is empty."; return; }
    read -rp "Recent lines to show [Enter = 50, 'all' = everything]: " n; n=${n:-50}
    if [[ $n == all ]]; then cat "$LOG_FILE"
    elif [[ $n =~ ^[1-9][0-9]*$ ]]; then tail -n "$n" "$LOG_FILE"
    else err "Invalid number '$n'."; return; fi
    log INFO "Viewed activity log"
}

clear_log() {
    confirm "Permanently delete all log entries?" || { echo "Cancelled."; return; }
    : > "$LOG_FILE" 2>/dev/null && { echo "Activity log cleared."; log INFO "Activity log cleared by user"; } \
                                 || err "Could not clear the log file."
}

quit_app() { log INFO "Application exited"; echo -e "\nGoodbye!"; exit 0; }

# ---------- main ----------
main() {
    local m c
    for c in ps df awk date uname; do
        command -v "$c" >/dev/null 2>&1 || m+="$c "
    done
    [[ -z $m ]] || { err "Missing required utilities: $m"; exit 1; }
    mkdir -p "$LOG_DIR" 2>/dev/null && touch "$LOG_FILE" 2>/dev/null || {
        LOG_DIR="/tmp/sysmon_$(id -un)"; LOG_FILE="$LOG_DIR/activity.log"
        mkdir -p "$LOG_DIR" && touch "$LOG_FILE" || { err "Cannot create a log file."; exit 1; }; }
    log INFO "Application started by $(id -un)"
    trap quit_app INT TERM

    while true; do
        cat << 'MENU'

==============================================
   LINUX SYSTEM HEALTH & RESOURCE MONITOR
==============================================
 1) Display system information
 2) Monitor CPU utilization
 3) Monitor memory utilization
 4) Monitor disk utilization
 5) View and search running processes
 6) Terminate a process
 7) Generate a system health report
 8) View activity history
 9) Clear activity log
10) Exit
----------------------------------------------
MENU
        read -rp "Enter your choice [1-10]: " c || quit_app
        echo
        case $c in
            1) sec_sysinfo; log INFO "Displayed system information" ;;
            2) monitor CPU CPU_T sec_cpu ;;
            3) monitor Memory MEM_T sec_mem ;;
            4) monitor Disk DISK_T sec_disk ;;
            5) view_processes ;;
            6) terminate_process ;;
            7) generate_report ;;
            8) view_log ;;
            9) clear_log ;;
            10) quit_app ;;
            *) err "Invalid choice '$c'. Enter a number from 1 to 10." ;;
        esac
        [[ $c == 5 ]] || { echo; read -rp "Press Enter to continue..." _; }
    done
}
main
