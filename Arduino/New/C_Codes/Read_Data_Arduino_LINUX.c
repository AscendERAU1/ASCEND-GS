#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

#define MAX_PORTS 16
#define MAX_LINE  512
#define CELL_LEN  24

static volatile sig_atomic_t running = 1;
static void handle_sigint(int sig) { (void)sig; running = 0; }

/* ---------- column layout: edit this list to add/remove variables ---------- */
static const char *COLS[] = {
    "D1_ms", "nt","ox","oy","oz","gx","gy","gz","lx","ly","lz",
    "D2_ms", "mx","my","mz","ax","ay","az","vx","vy","vz","it",
    "D3_ms", "iax","iay","iaz","igx","igy","igz","co2","st","rh","ba","bp","bt","uv","als",
    "D4_ms", "f1","f2","f3","f4","f5","f6","f7","f8","cl","nir","et","ep","eh","eg","ea"
};
#define NUM_COLS (int)(sizeof(COLS) / sizeof(COLS[0]))

static char cells[NUM_COLS][CELL_LEN];
static int  row_has_data = 0;
static long row_count = 0;

static int find_col(const char *name) {
    for (int i = 0; i < NUM_COLS; i++)
        if (strcmp(COLS[i], name) == 0) return i;
    return -1;
}

static void clear_row(void) {
    memset(cells, 0, sizeof(cells));
    row_has_data = 0;
}

static void write_header(FILE *f) {
    for (int i = 0; i < NUM_COLS; i++)
        fprintf(f, "%s%s", COLS[i], i < NUM_COLS - 1 ? "," : "\n");
    fflush(f);
}

static void flush_row(FILE *f) {
    if (!row_has_data) return;
    for (int i = 0; i < NUM_COLS; i++)
        fprintf(f, "%s%s", cells[i], i < NUM_COLS - 1 ? "," : "\n");
    fflush(f);
    row_count++;
    printf("Logged row %ld\n", row_count);
    clear_row();
}

/* Handles one line. Works whether the Arduino prints one token per line
   or a whole packet on one line, because it splits on commas either way. */
static void handle_line(char *line, FILE *f) {
    static char pending_ms_col[16] = "";   // set after seeing D1..D4
    char *save = NULL;
    for (char *tok = strtok_r(line, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
        char *colon = strchr(tok, ':');

        if (!colon) {
            if (tok[0] == 'D' && tok[1] >= '1' && tok[1] <= '4' && tok[2] == '\0') {
                if (tok[1] == '1') flush_row(f);          // new burst: write the old row
                snprintf(pending_ms_col, sizeof(pending_ms_col), "%s_ms", tok);
            } else if (pending_ms_col[0]) {
                int c = find_col(pending_ms_col);          // token after the ID = millis
                if (c >= 0) { snprintf(cells[c], CELL_LEN, "%s", tok); row_has_data = 1; }
                pending_ms_col[0] = '\0';
            }
            continue;
        }

        *colon = '\0';                                     // tok = tag, colon+1 = value
        int c = find_col(tok);
        if (c < 0) continue;                               // unknown tag (e.g. GPS line), ignore
        snprintf(cells[c], CELL_LEN, "%s", colon + 1);
        row_has_data = 1;
        if (strcmp(tok, "ea") == 0) flush_row(f);          // last tag of D4: row complete
    }
}

/* ---------- serial helpers (unchanged) ---------- */
void find_serial_ports(char ports[][256], int *count) {
    *count = 0;
    DIR *dir = opendir("/dev");
    if (!dir) { perror("opendir"); return; }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && *count < MAX_PORTS) {
        if (strncmp(entry->d_name, "ttyUSB", 6) == 0 ||
            strncmp(entry->d_name, "ttyACM", 6) == 0) {
            snprintf(ports[*count], 256, "/dev/%.250s", entry->d_name);
            (*count)++;
        }
    }
    closedir(dir);
}

int setup_serial(int fd) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) { perror("tcgetattr"); return -1; }
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tty.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
    tty.c_cflag |= CS8 | CLOCAL | CREAD;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 0;
    if (tcsetattr(fd, TCSANOW, &tty) != 0) { perror("tcsetattr"); return -1; }
    return 0;
}

int read_line(int fd, char *line, int max_len) {
    int i = 0;
    char c;
    while (i < max_len - 1) {
        ssize_t n = read(fd, &c, 1);
        if (n < 0) {
            if (errno == EINTR) { if (!running) return -1; continue; }
            perror("read");
            return -1;
        }
        if (n == 0) return -1;
        if (c == '\n') break;
        if (c == '\r') continue;
        line[i++] = c;
    }
    line[i] = '\0';
    return i;
}

int main(void) {
    char ports[MAX_PORTS][256];
    int port_count = 0;

    find_serial_ports(ports, &port_count);
    if (port_count == 0) { printf("No serial ports found.\n"); return 1; }

    printf("\nAvailable serial ports:\n");
    for (int i = 0; i < port_count; i++) printf("[%d] %s\n", i, ports[i]);

    int idx;
    printf("\nSelect port to read: ");
    if (scanf("%d", &idx) != 1) { printf("Invalid input.\n"); return 1; }
    if (idx < 0 || idx >= port_count) { printf("Invalid selection.\n"); return 1; }

    int fd = open(ports[idx], O_RDONLY | O_NOCTTY);
    if (fd < 0) { perror("open"); return 1; }
    if (setup_serial(fd) < 0) { close(fd); return 1; }

    FILE *out_file = fopen("serial_log.csv", "a");
    if (!out_file) { perror("Error opening output file"); close(fd); return 1; }

    // Header only if the file is empty, so it's never printed twice
    fseek(out_file, 0, SEEK_END);
    if (ftell(out_file) == 0) write_header(out_file);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa, NULL);

    clear_row();
    printf("\nLogging %s to serial_log.csv... Press Ctrl+C to stop.\n\n", ports[idx]);

    char line[MAX_LINE];
    while (running) {
        int len = read_line(fd, line, sizeof(line));
        if (len < 0) break;
        if (len == 0) continue;
        handle_line(line, out_file);
    }

    flush_row(out_file);   // write any partial row before exiting
    printf("\nStopping, closing files.\n");
    fclose(out_file);
    close(fd);
    return 0;
}