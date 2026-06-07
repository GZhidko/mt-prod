#define _GNU_SOURCE
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "src/acct.h"
#include "src/acct_method.h"
#include "src/acct_method_detail.h"
#include "src/acct_scope.h"
#include "src/cfg.h"
#include "src/filter.h"
#include "src/gauge.h"
#include "src/lhstat.h"
#include "src/log.h"
#include "src/pretty.h"
#include "src/session.h"
#include "src/shape.h"
#include "src/sweetspot.h"
#include "src/uammsg.h"
#include "src/uamsrv.h"

#define UAM_TEST_THREADS 16
#define UAM_TEST_DURATION_SEC 15
#define UAM_STALL_TIMEOUT_SEC 2
#define UAM_IP_WINDOW 16

int global_alive_flag = 1;
int process_qnty = 0;
sw_socket_handler_t **global_socket_handlers = NULL;
char inner_gw_mac[8] = {0,0,0,0,0,0,0,0};
char outer_gw_mac[8] = {0,0,0,0,0,0,0,0};

static volatile int g_running = 1;
static volatile int g_failed = 0;
static volatile unsigned long g_progress[UAM_TEST_THREADS + 1];
static const char *g_filter_name = "authonly";

typedef struct uam_client_ctx_t {
    int sock;
    struct sockaddr_in addr;
    char recv_buf[SW_UAM_MSG_SIZE];
} uam_client_ctx_t;

static unsigned long load_progress(int idx) {
    return __atomic_load_n(&g_progress[idx], __ATOMIC_SEQ_CST);
}

static void bump_progress(int idx) {
    __atomic_add_fetch(&g_progress[idx], 1, __ATOMIC_SEQ_CST);
}

static int init_uam_client(uam_client_ctx_t *ctx) {
    socklen_t addr_len = sizeof(ctx->addr);

    memset(ctx, 0, sizeof(*ctx));
    ctx->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->sock == -1) {
        fprintf(stderr, "socket failed: %s\n", strerror(errno));
        return -1;
    }

    ctx->addr.sin_family = AF_INET;
    ctx->addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ctx->addr.sin_port = htons(0);
    if (bind(ctx->sock, (struct sockaddr *)&ctx->addr, sizeof(ctx->addr)) == -1) {
        fprintf(stderr, "bind failed: %s\n", strerror(errno));
        close(ctx->sock);
        ctx->sock = -1;
        return -1;
    }

    if (getsockname(ctx->sock, (struct sockaddr *)&ctx->addr, &addr_len) == -1) {
        fprintf(stderr, "getsockname failed: %s\n", strerror(errno));
        close(ctx->sock);
        ctx->sock = -1;
        return -1;
    }

    struct timeval timeout = { .tv_sec = 0, .tv_usec = 200000 };
    setsockopt(ctx->sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    return 0;
}

static void destroy_uam_client(uam_client_ctx_t *ctx) {
    if (ctx->sock != -1) {
        close(ctx->sock);
        ctx->sock = -1;
    }
}

static int dispatch_uam_message(uam_client_ctx_t *ctx, char **argv) {
    char *cyphertext = NULL;
    int msg_len;
    char response_buf[SW_UAM_MSG_SIZE];

    msg_len = sw_uam_build_msg(argv, &cyphertext);
    if (msg_len == -1) {
        return -1;
    }

    if (uam_server.proc(&uam_server, NULL, msg_len, &ctx->addr, cyphertext) == -1) {
        return -1;
    }

    if (recvfrom(ctx->sock, response_buf, sizeof(response_buf), 0, NULL, NULL) == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -1;
        }
        return -1;
    }

    return 0;
}

static int write_test_filter(const char *path) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }

    fprintf(fp, "in all pass\nout all pass\n");
    fclose(fp);
    return 0;
}

static int write_test_config(const char *path, const char *filter_dir, const char *acct_detail) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }

    fprintf(fp,
            "user-networks 127.0.0.1/24\n"
            "uam-server-port 3993\n"
            "uam-secret 1234567890\n"
            "acct-detail-file %s\n"
            "acct-interim-interval 1\n"
            "filter-dir %s\n"
            "filter-anonymous %s\n"
            "local-networks 127.0.0.1/28\n",
            acct_detail, filter_dir, g_filter_name);

    fclose(fp);
    return 0;
}

static int build_paths(char *config_path, size_t config_sz,
                       char *log_path, size_t log_sz,
                       char *detail_path, size_t detail_sz,
                       char *filter_path, size_t filter_sz,
                       char *filter_file_path, size_t filter_file_sz) {
    char cwd[4096];

    if (!getcwd(cwd, sizeof(cwd))) {
        fprintf(stderr, "getcwd failed: %s\n", strerror(errno));
        return -1;
    }

    snprintf(config_path, config_sz, "%s/tests/uamConcurrencyTest/generated.conf", cwd);
    snprintf(log_path, log_sz, "%s/tests/uamConcurrencyTest/uamConcurrencyTest.log", cwd);
    snprintf(detail_path, detail_sz, "%s/tests/uamConcurrencyTest/detail.log", cwd);
    snprintf(filter_path, filter_sz, "%s/tests/uamConcurrencyTest/filters", cwd);
    snprintf(filter_file_path, filter_file_sz, "%s/authonly", filter_path);

    return 0;
}

static void *maintenance_thread(void *arg) {
    const char *filter_dir = (const char *)arg;

    while (__atomic_load_n(&g_running, __ATOMIC_SEQ_CST)) {
        time_t now = time(NULL);
        sw_lhstat_do(now);
        sw_gauge_expire();
        sw_session_expire();
        sw_acct_commit(0);
        sw_filter_reload((char *)filter_dir, now);
        bump_progress(UAM_TEST_THREADS);
        usleep(1000);
    }

    return NULL;
}

static void *uam_worker_thread(void *arg) {
    long worker_id = (long)arg;
    unsigned int seed = (unsigned int)(time(NULL) ^ (worker_id * 2654435761u));
    uint32_t ip_count;
    uam_client_ctx_t client;
    char serial[16];
    char stateid[16];
    char ipbuf[32];
    char interim[16];
    char octets_limit[32];
    char bps_limit[32];
    char retention[16];
    char pub_port[16];
    char *argv[SW_UAM_ARG_MAX];

    ip_count = sw_netset_size(global_ns);
    if (ip_count == 0) {
        __atomic_store_n(&g_failed, 1, __ATOMIC_SEQ_CST);
        return NULL;
    }
    if (init_uam_client(&client) == -1) {
        __atomic_store_n(&g_failed, 1, __ATOMIC_SEQ_CST);
        return NULL;
    }

    while (__atomic_load_n(&g_running, __ATOMIC_SEQ_CST)) {
        uint32_t ip_offset = rand_r(&seed) % (ip_count < UAM_IP_WINDOW ? ip_count : UAM_IP_WINDOW);
        uint32_t ip = sw_netset_ip(global_ns, ip_offset);
        int action = rand_r(&seed) % 8;

        snprintf(serial, sizeof(serial), "%u", rand_r(&seed));
        snprintf(stateid, sizeof(stateid), "0");
        snprintf(ipbuf, sizeof(ipbuf), "%s", sw_pretty_ip(ip));
        snprintf(interim, sizeof(interim), "%u", 1 + (rand_r(&seed) % 2));
        snprintf(octets_limit, sizeof(octets_limit), "%u", 1000000 + (rand_r(&seed) % 10000));
        snprintf(bps_limit, sizeof(bps_limit), "%u", 100000 + (rand_r(&seed) % 1000));
        snprintf(retention, sizeof(retention), "%u", 1 + (rand_r(&seed) % 3));
        snprintf(pub_port, sizeof(pub_port), "%u", 1024 + (rand_r(&seed) % 2048));

        memset(argv, 0, sizeof(argv));
        argv[0] = serial;
        argv[1] = stateid;

        switch (action) {
        case 0:
            argv[2] = "UP";
            argv[3] = ipbuf;
            argv[4] = (char *)g_filter_name;
            argv[5] = interim;
            argv[6] = octets_limit;
            argv[7] = octets_limit;
            argv[8] = bps_limit;
            argv[9] = bps_limit;
            argv[10] = retention;
            argv[11] = retention;
            argv[12] = "";
            argv[13] = "";
            break;
        case 1:
            argv[2] = "DN";
            argv[3] = ipbuf;
            argv[4] = "";
            argv[5] = "";
            argv[6] = "";
            break;
        case 2:
            argv[2] = "ST";
            argv[3] = ipbuf;
            break;
        case 3:
            argv[2] = "LI";
            argv[3] = ipbuf;
            break;
        case 4:
            argv[2] = "CN";
            argv[3] = ipbuf;
            break;
        case 5:
            argv[2] = "PE";
            argv[3] = ipbuf;
            break;
        case 6:
            argv[2] = "RE";
            argv[3] = ipbuf;
            argv[4] = retention;
            break;
        case 7:
            argv[2] = "IP";
            argv[3] = ipbuf;
            argv[4] = pub_port;
            break;
        }

        dispatch_uam_message(&client, argv);
        bump_progress((int)worker_id);
    }

    destroy_uam_client(&client);
    return NULL;
}

static int setup_environment(const char *config_path, const char *log_path) {
    if (sw_log_init((char *)log_path) == -1) {
        fprintf(stderr, "sw_log_init failed for %s\n", log_path);
        return -1;
    }

    if (sw_config_load(config_path) < 0) {
        fprintf(stderr, "sw_config_load failed for %s\n", config_path);
        return -1;
    }

    if (sw_lhstat_create() == -1) return -1;
    if (sw_filter_create(sw_config_get("filter-dir", SW_FILTER_DIR)) == -1) return -1;
    if (sw_session_create() == -1) return -1;
    if (sw_gauge_create() < 0) return -1;
    if (sw_acct_scope_create() == -1) return -1;
    if (sw_acct_method_create() == -1) return -1;
    if (sw_acct_method_detail_create() == -1) return -1;
    if (sw_shape_create() == -1) return -1;
    if (uam_server.create(&uam_server) == -1) return -1;

    return 0;
}

static void cleanup_environment(void) {
    if (uam_server.destroy) {
        uam_server.destroy(&uam_server);
    }
    sw_session_destroy();
    sw_gauge_destroy();
    sw_shape_destroy();
    sw_acct_commit(1);
    sw_acct_method_destroy();
    sw_lhstat_destroy();
}

static int timed_join(pthread_t tid, const char *name) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 2;

    if (pthread_timedjoin_np(tid, NULL, &ts) != 0) {
        fprintf(stderr, "timed out waiting for %s\n", name);
        return -1;
    }

    return 0;
}

static void reset_test_state(void) {
    g_running = 1;
    g_failed = 0;
    global_alive_flag = 1;
    memset((void *)g_progress, 0, sizeof(g_progress));
}

static int run_single_round(const char *config_path, const char *log_path) {
    pthread_t workers[UAM_TEST_THREADS];
    pthread_t maintenance;
    unsigned long last_total = 0;
    time_t last_change = time(NULL);
    reset_test_state();

    if (setup_environment(config_path, log_path) == -1) {
        return 1;
    }

    for (long i = 0; i < UAM_TEST_THREADS; i++) {
        if (pthread_create(&workers[i], NULL, uam_worker_thread, (void *)i) != 0) {
            fprintf(stderr, "failed to create worker %ld\n", i);
            __atomic_store_n(&g_failed, 1, __ATOMIC_SEQ_CST);
            __atomic_store_n(&g_running, 0, __ATOMIC_SEQ_CST);
            break;
        }
    }

    if (!__atomic_load_n(&g_failed, __ATOMIC_SEQ_CST) &&
        pthread_create(&maintenance, NULL, maintenance_thread,
                       (void *)sw_config_get("filter-dir", SW_FILTER_DIR)) != 0) {
        fprintf(stderr, "failed to create maintenance thread\n");
        __atomic_store_n(&g_failed, 1, __ATOMIC_SEQ_CST);
        __atomic_store_n(&g_running, 0, __ATOMIC_SEQ_CST);
    }

    for (int elapsed = 0; elapsed < UAM_TEST_DURATION_SEC && !__atomic_load_n(&g_failed, __ATOMIC_SEQ_CST); elapsed++) {
        sleep(1);

        unsigned long total = 0;
        for (int i = 0; i <= UAM_TEST_THREADS; i++) {
            total += load_progress(i);
        }

        if (total != last_total) {
            last_total = total;
            last_change = time(NULL);
        } else if (time(NULL) - last_change >= UAM_STALL_TIMEOUT_SEC) {
            fprintf(stderr, "progress stalled, possible deadlock\n");
            __atomic_store_n(&g_failed, 1, __ATOMIC_SEQ_CST);
            break;
        }
    }

    __atomic_store_n(&g_running, 0, __ATOMIC_SEQ_CST);
    global_alive_flag = 0;

    for (int i = 0; i < UAM_TEST_THREADS; i++) {
        if (timed_join(workers[i], "uam worker") != 0) {
            __atomic_store_n(&g_failed, 1, __ATOMIC_SEQ_CST);
        }
    }

    if (timed_join(maintenance, "maintenance worker") != 0) {
        __atomic_store_n(&g_failed, 1, __ATOMIC_SEQ_CST);
    }

    cleanup_environment();

    if (__atomic_load_n(&g_failed, __ATOMIC_SEQ_CST)) {
        return 1;
    }

    return 0;
}

int main(int argc, char **argv) {
    int rounds = 1;
    int passed = 0;
    int failed = 0;
    char config_path[4096];
    char log_path[4096];
    char detail_path[4096];
    char filter_path[4096];
    char filter_file_path[4096];

    if (argc == 3 && strcmp(argv[1], "--rounds") == 0) {
        rounds = atoi(argv[2]);
        if (rounds < 1) {
            fprintf(stderr, "invalid rounds value: %s\n", argv[2]);
            return 1;
        }
    } else if (argc != 1) {
        fprintf(stderr, "usage: %s [--rounds N]\n", argv[0]);
        return 1;
    }

    if (build_paths(config_path, sizeof(config_path),
                    log_path, sizeof(log_path),
                    detail_path, sizeof(detail_path),
                    filter_path, sizeof(filter_path),
                    filter_file_path, sizeof(filter_file_path)) == -1) {
        return 1;
    }

    mkdir(filter_path, 0777);
    unlink(detail_path);

    if (write_test_filter(filter_file_path) == -1) {
        return 1;
    }

    if (write_test_config(config_path, filter_path, detail_path) == -1) {
        return 1;
    }

    for (int round = 1; round <= rounds; round++) {
        int rc = run_single_round(config_path, log_path);
        if (rc == 0) {
            passed++;
            printf("round %d/%d: passed\n", round, rounds);
        } else {
            failed++;
            printf("round %d/%d: failed\n", round, rounds);
        }
    }

    unlink(config_path);

    printf("uamConcurrencyTest summary: passed=%d failed=%d rounds=%d\n",
           passed, failed, rounds);

    return failed == 0 ? 0 : 1;
}
