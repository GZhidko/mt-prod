#define _GNU_SOURCE
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
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
#include "src/filter_match.h"
#include "src/gauge.h"
#include "src/lhstat.h"
#include "src/log.h"
#include "src/nat_dst.h"
#include "src/nat_src.h"
#include "src/pretty.h"
#include "src/session.h"
#include "src/shape.h"
#include "src/sweetspot.h"
#include "src/tuple.h"
#include "src/tuple_ip.h"
#include "src/tuple_tcp.h"
#include "src/uammsg.h"
#include "src/uamsrv.h"

#define DEFAULT_DURATION_SEC 300
#define DEFAULT_SESSION_TARGET 10000
#define DEFAULT_UAM_THREADS 16
#define DEFAULT_TRAFFIC_THREADS 8
#define DEFAULT_ROUNDS 1
#define PROGRESS_STALL_TIMEOUT_SEC 5
#define SAMPLE_VERIFY_BATCH 64
#define MAX_THREADS_TOTAL 256

int global_alive_flag = 1;
int process_qnty = 0;
sw_socket_handler_t **global_socket_handlers = NULL;
char inner_gw_mac[8] = {0, 0, 0, 0, 0, 0, 0, 0};
char outer_gw_mac[8] = {0, 0, 0, 0, 0, 0, 0, 0};

typedef struct stress_cfg_t {
    int rounds;
    int duration_sec;
    int session_target;
    int uam_threads;
    int traffic_threads;
} stress_cfg_t;

typedef struct uam_client_ctx_t {
    int sock;
    struct sockaddr_in addr;
} uam_client_ctx_t;

typedef struct thread_ctx_t {
    int progress_idx;
    int thread_id;
} thread_ctx_t;

static stress_cfg_t g_cfg = {
    DEFAULT_ROUNDS,
    DEFAULT_DURATION_SEC,
    DEFAULT_SESSION_TARGET,
    DEFAULT_UAM_THREADS,
    DEFAULT_TRAFFIC_THREADS
};

static volatile int g_running = 1;
static volatile int g_failed = 0;
static volatile unsigned long *g_progress = NULL;
static time_t g_round_started = 0;
static uint32_t *g_ip_pool = NULL;
static const char *g_filter_name = "stressshape";
static const uint32_t g_service_ip = 0xcb00710aU; /* 203.0.113.10 */
static pthread_mutex_t g_uam_build_lock = PTHREAD_MUTEX_INITIALIZER;

static unsigned long load_progress(int idx) {
    return __atomic_load_n(&g_progress[idx], __ATOMIC_SEQ_CST);
}

static void bump_progress(int idx) {
    __atomic_add_fetch(&g_progress[idx], 1, __ATOMIC_SEQ_CST);
}

static void set_failed(const char *reason) {
    if (!__atomic_exchange_n(&g_failed, 1, __ATOMIC_SEQ_CST)) {
        fprintf(stderr, "%s\n", reason);
    }
    __atomic_store_n(&g_running, 0, __ATOMIC_SEQ_CST);
}

static int parse_int_arg(const char *value, int *target, const char *name) {
    char *end = NULL;
    long parsed = strtol(value, &end, 10);

    if (!value[0] || (end && *end) || parsed < 1 || parsed > 10000000) {
        fprintf(stderr, "invalid %s: %s\n", name, value);
        return -1;
    }

    *target = (int)parsed;
    return 0;
}

static int is_decimal_string(const char *value) {
    if (!value || !*value) {
        return 0;
    }

    for (const char *p = value; *p; p++) {
        if (*p < '0' || *p > '9') {
            return 0;
        }
    }

    return 1;
}

static int validate_uam_response(const char *op, char **argv) {
    int status_idx = 0;
    int payload_idx = 2;
    const char *status;
    const char *reason;

    if (argv[0] && is_decimal_string(argv[0])) {
        status_idx = 2;
        payload_idx = 4;
    }

    status = argv[status_idx];
    reason = argv[status_idx + 2];
    if (!status) {
        return -1;
    }

    if (strcmp(op, "UP") == 0 || strcmp(op, "DN") == 0) {
        return strcmp(status, "OK") == 0 ? 0 : -1;
    }

    if (strcmp(op, "ST") == 0) {
        if (strcmp(status, "OK") != 0) {
            return -1;
        }
        if (!argv[payload_idx] ||
            (strcmp(argv[payload_idx], "UP") != 0 &&
             strcmp(argv[payload_idx], "DN") != 0)) {
            return -1;
        }
        return argv[payload_idx + 1] && is_decimal_string(argv[payload_idx + 1]) ? 0 : -1;
    }

    if (strcmp(op, "LI") == 0) {
        if (strcmp(status, "OK") == 0) {
            for (int i = 0; i < 6; i++) {
                if (!argv[payload_idx + i] || !is_decimal_string(argv[payload_idx + i])) {
                    return -1;
                }
            }
            return 0;
        }
        return reason && strcmp(reason, "getting-limits-failure") == 0 ? 0 : -1;
    }

    if (strcmp(op, "CN") == 0) {
        if (strcmp(status, "OK") == 0) {
            for (int i = 0; i < 6; i++) {
                if (!argv[payload_idx + i] || !is_decimal_string(argv[payload_idx + i])) {
                    return -1;
                }
            }
            return 0;
        }
        return reason && strcmp(reason, "getting-currents-failure") == 0 ? 0 : -1;
    }

    if (strcmp(op, "PE") == 0) {
        if (strcmp(status, "OK") == 0) {
            return argv[payload_idx] && argv[payload_idx + 1] &&
                   is_decimal_string(argv[payload_idx]) &&
                   is_decimal_string(argv[payload_idx + 1]) ? 0 : -1;
        }
        return reason && strcmp(reason, "getting-peaks-failure") == 0 ? 0 : -1;
    }

    if (strcmp(op, "RE") == 0) {
        if (strcmp(status, "OK") == 0) {
            return argv[payload_idx] && is_decimal_string(argv[payload_idx]) ? 0 : -1;
        }
        return reason && strcmp(reason, "retention-operation-failure") == 0 ? 0 : -1;
    }

    return strcmp(status, "OK") == 0 ? 0 : -1;
}

static int parse_args(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--rounds") == 0 && i + 1 < argc) {
            if (parse_int_arg(argv[++i], &g_cfg.rounds, "rounds") == -1) return -1;
        } else if (strcmp(argv[i], "--duration") == 0 && i + 1 < argc) {
            if (parse_int_arg(argv[++i], &g_cfg.duration_sec, "duration") == -1) return -1;
        } else if (strcmp(argv[i], "--sessions") == 0 && i + 1 < argc) {
            if (parse_int_arg(argv[++i], &g_cfg.session_target, "sessions") == -1) return -1;
        } else if (strcmp(argv[i], "--uam-threads") == 0 && i + 1 < argc) {
            if (parse_int_arg(argv[++i], &g_cfg.uam_threads, "uam-threads") == -1) return -1;
        } else if (strcmp(argv[i], "--traffic-threads") == 0 && i + 1 < argc) {
            if (parse_int_arg(argv[++i], &g_cfg.traffic_threads, "traffic-threads") == -1) return -1;
        } else {
            fprintf(stderr,
                    "usage: %s [--rounds N] [--duration SEC] [--sessions N] "
                    "[--uam-threads N] [--traffic-threads N]\n",
                    argv[0]);
            return -1;
        }
    }

    if (g_cfg.uam_threads + g_cfg.traffic_threads + 1 > MAX_THREADS_TOTAL) {
        fprintf(stderr, "too many threads requested\n");
        return -1;
    }

    return 0;
}

static int init_uam_client(uam_client_ctx_t *ctx) {
    socklen_t addr_len = sizeof(ctx->addr);
    struct timeval timeout = { .tv_sec = 1, .tv_usec = 0 };

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

    if (setsockopt(ctx->sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
        fprintf(stderr, "setsockopt failed: %s\n", strerror(errno));
        close(ctx->sock);
        ctx->sock = -1;
        return -1;
    }

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
    char local_msg[SW_UAM_MSG_SIZE];
    char response_buf[SW_UAM_MSG_SIZE];
    char *response_argv[SW_UAM_ARG_MAX];
    int msg_len;
    int response_len;
    int rc = -1;

    pthread_mutex_lock(&g_uam_build_lock);
    msg_len = sw_uam_build_msg(argv, &cyphertext);
    if (msg_len == -1) {
        pthread_mutex_unlock(&g_uam_build_lock);
        return -1;
    }
    memcpy(local_msg, cyphertext, (size_t)msg_len);
    pthread_mutex_unlock(&g_uam_build_lock);

    if (uam_server.proc(&uam_server, NULL, msg_len, &ctx->addr, local_msg) == -1) {
        goto out;
    }

    response_len = recvfrom(ctx->sock, response_buf, sizeof(response_buf), 0, NULL, NULL);
    if (response_len == -1) {
        goto out;
    }

    if (sw_uam_parse_msg(response_argv, response_buf, response_len) == -1) {
        goto out;
    }

    if (validate_uam_response(argv[2], response_argv) == -1) {
        goto out;
    }

    rc = 0;

out:
    return rc;
}

static sw_tuple_t *make_tcp_tuple(uint32_t ip, uint16_t port, uint8_t flags) {
    sw_tuple_t *ip_tuple = NULL;
    sw_tuple_t *tcp_tuple = NULL;
    sw_tuple_ip_t *ip_prv = NULL;
    sw_tuple_tcp_t *tcp_prv = NULL;

    ip_tuple = malloc(sizeof(*ip_tuple));
    tcp_tuple = malloc(sizeof(*tcp_tuple));
    ip_prv = malloc(sizeof(*ip_prv));
    tcp_prv = malloc(sizeof(*tcp_prv));
    if (!ip_tuple || !tcp_tuple || !ip_prv || !tcp_prv) {
        free(ip_tuple);
        free(tcp_tuple);
        free(ip_prv);
        free(tcp_prv);
        return NULL;
    }

    memset(ip_tuple, 0, sizeof(*ip_tuple));
    memset(tcp_tuple, 0, sizeof(*tcp_tuple));
    memset(ip_prv, 0, sizeof(*ip_prv));
    memset(tcp_prv, 0, sizeof(*tcp_prv));

    ip_tuple->protocol = IPPROTO_IP;
    ip_tuple->prv = ip_prv;
    ip_tuple->next = tcp_tuple;

    tcp_tuple->protocol = IPPROTO_TCP;
    tcp_tuple->prv = tcp_prv;
    tcp_tuple->prev = ip_tuple;

    ip_prv->ip = htonl(ip);
    tcp_prv->port = htons(port);
    tcp_prv->flags = flags;

    return ip_tuple;
}

static int verify_nat_roundtrip(sw_tuple_t *original, sw_tuple_t *restored) {
    sw_tuple_ip_t *src_ip = original->prv;
    sw_tuple_ip_t *dst_ip = restored->prv;
    sw_tuple_tcp_t *src_tcp = original->next->prv;
    sw_tuple_tcp_t *dst_tcp = restored->next->prv;

    if (src_ip->ip != dst_ip->ip || src_tcp->port != dst_tcp->port) {
        return -1;
    }

    return 0;
}

static int replay_single_flow(uint32_t ip, unsigned int *seed) {
    sw_tuple_t *src = NULL;
    sw_tuple_t *dst = NULL;
    sw_tuple_t *pub = NULL;
    sw_tuple_t *undo = NULL;
    sw_tuple_t *reply = NULL;
    sw_filter_rule_t *rule = NULL;
    int rc = -1;
    int port = ((*seed = rand_r(seed)) % 2) ? 80 : 443;
    int pkt_len = 300 + ((*seed = rand_r(seed)) % 1200);
    int delaytime;

    src = make_tcp_tuple(ip, (uint16_t)(1024 + ((*seed = rand_r(seed)) % 50000)), TH_SYN | TH_ACK);
    dst = make_tcp_tuple(g_service_ip, (uint16_t)port, TH_ACK);
    if (!src || !dst) {
        goto out;
    }

    if (sw_filter_match(&rule, src, dst, SW_FILTER_MATCH_INWARD) != 1 || !rule) {
        goto out;
    }

    if (sw_gauge_update(src, pkt_len, SW_GAUGE_DIR_IN) == -1) {
        goto out;
    }

    if (sw_nat_src_do(&pub, src) == -1 || !pub) {
        goto out;
    }

    delaytime = sw_shape_schedule(src, pkt_len, rule->shape.rate, SW_SHAPE_IN, 0);
    if (delaytime < 0) {
        goto out;
    }

    reply = make_tcp_tuple(g_service_ip, (uint16_t)port, TH_ACK);
    if (!reply) {
        goto out;
    }

    if (sw_nat_src_undo(&undo, pub) == -1 || !undo) {
        goto out;
    }

    if (verify_nat_roundtrip(src, undo) == -1) {
        goto out;
    }

    rule = NULL;
    if (sw_filter_match(&rule, reply, undo, SW_FILTER_MATCH_OUTWARD) != 1 || !rule) {
        goto out;
    }

    if (sw_gauge_update(undo, pkt_len, SW_GAUGE_DIR_OUT) == -1) {
        goto out;
    }

    delaytime = sw_shape_schedule(undo, pkt_len, rule->shape.rate, SW_SHAPE_OUT, 0);
    if (delaytime < 0) {
        goto out;
    }

    rc = 0;

out:
    if (src) sw_tuple_free(src);
    if (dst) sw_tuple_free(dst);
    if (pub) sw_tuple_free(pub);
    if (undo) sw_tuple_free(undo);
    if (reply) sw_tuple_free(reply);
    return rc;
}

static int write_test_filter(const char *path) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }

    fprintf(fp,
            "shape rate 4096 in\n"
            "shape rate 4096 out\n");
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
            "user-networks 10.20.0.1/18\n"
            "snat-public-networks 198.18.0.1/24\n"
            "uam-server-port 3994\n"
            "uam-secret 1234567890\n"
            "acct-detail-file %s\n"
            "acct-interim-interval 1\n"
            "filter-dir %s\n"
            "filter-anonymous %s\n"
            "local-networks 169.254.128.254/30\n",
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

    snprintf(config_path, config_sz, "%s/tests/coreTest/generated_new.conf", cwd);
    snprintf(log_path, log_sz, "%s/tests/coreTest/coreTest_new.log", cwd);
    snprintf(detail_path, detail_sz, "%s/tests/coreTest/detail_new.log", cwd);
    snprintf(filter_path, filter_sz, "%s/tests/coreTest/filters_new", cwd);
    snprintf(filter_file_path, filter_file_sz, "%s/%s", filter_path, g_filter_name);

    return 0;
}

static int prepare_artifacts(const char *config_path,
                             const char *detail_path,
                             const char *filter_path,
                             const char *filter_file_path) {
    mkdir(filter_path, 0777);
    unlink(detail_path);

    if (write_test_filter(filter_file_path) == -1) {
        return -1;
    }

    if (write_test_config(config_path, filter_path, detail_path) == -1) {
        return -1;
    }

    return 0;
}

static int build_ip_pool(void) {
    int available = (int)sw_netset_size(global_ns);

    if (available < g_cfg.session_target) {
        fprintf(stderr, "configured user network has only %d IPs, need %d\n",
                available, g_cfg.session_target);
        return -1;
    }

    g_ip_pool = calloc((size_t)g_cfg.session_target, sizeof(*g_ip_pool));
    if (!g_ip_pool) {
        fprintf(stderr, "failed to allocate test IP pool\n");
        return -1;
    }

    for (int i = 0; i < g_cfg.session_target; i++) {
        g_ip_pool[i] = sw_netset_ip(global_ns, i);
    }

    return 0;
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
    if (sw_nat_dst_create() == -1) return -1;
    if (sw_acct_scope_create() == -1) return -1;
    if (sw_acct_method_create() == -1) return -1;
    if (sw_acct_method_detail_create() == -1) return -1;
    if (sw_shape_create() == -1) return -1;
    if (sw_shape_queue_create(0) == -1) return -1;
    if (sw_shape_queue_create(1) == -1) return -1;
    if (uam_server.create(&uam_server) == -1) return -1;
    if (build_ip_pool() == -1) return -1;

    return 0;
}

static void cleanup_environment(void) {
    free(g_ip_pool);
    g_ip_pool = NULL;

    /*
     * Final accounting snapshot still dereferences live session/gauge/filter
     * structures, so drain the queue before those subsystems are destroyed.
     */
    sw_acct_commit(1);

    if (uam_server.destroy) {
        uam_server.destroy(&uam_server);
    }
    sw_shape_queue_destroy();
    sw_session_destroy();
    sw_gauge_destroy();
    sw_shape_destroy();
    sw_acct_method_destroy();
    sw_lhstat_destroy();
}

static void reset_round_state(void) {
    int slots = g_cfg.uam_threads + g_cfg.traffic_threads + 1;

    g_running = 1;
    g_failed = 0;
    global_alive_flag = 1;
    memset((void *)g_progress, 0, (size_t)slots * sizeof(*g_progress));
}

static int timed_join(pthread_t tid, const char *name) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 3;

    if (pthread_timedjoin_np(tid, NULL, &ts) != 0) {
        fprintf(stderr, "timed out waiting for %s\n", name);
        return -1;
    }

    return 0;
}

static int send_uam_up(uam_client_ctx_t *client, int idx, unsigned int *seed) {
    char serial[16];
    char stateid[16];
    char ipbuf[32];
    char interim[16];
    char octets_limit[32];
    char bps_limit[32];
    char duration[16];
    char idle[16];
    char *argv[16] = {0};

    snprintf(serial, sizeof(serial), "%u", rand_r(seed));
    snprintf(stateid, sizeof(stateid), "0");
    snprintf(ipbuf, sizeof(ipbuf), "%s", sw_pretty_ip(g_ip_pool[idx]));
    snprintf(interim, sizeof(interim), "%u", 1 + (rand_r(seed) % 2));
    snprintf(octets_limit, sizeof(octets_limit), "%u", 200000000 + (rand_r(seed) % 10000));
    snprintf(bps_limit, sizeof(bps_limit), "%u", 4096000 + (rand_r(seed) % 1000));
    snprintf(duration, sizeof(duration), "%u", 3600);
    snprintf(idle, sizeof(idle), "%u", 3600);

    argv[0] = serial;
    argv[1] = stateid;
    argv[2] = "UP";
    argv[3] = ipbuf;
    argv[4] = (char *)g_filter_name;
    argv[5] = interim;
    argv[6] = octets_limit;
    argv[7] = octets_limit;
    argv[8] = bps_limit;
    argv[9] = bps_limit;
    argv[10] = duration;
    argv[11] = idle;
    argv[12] = "";
    argv[13] = "";

    if (dispatch_uam_message(client, argv) == -1) {
        return -1;
    }

    return 0;
}

static int send_uam_dn(uam_client_ctx_t *client, int idx, unsigned int *seed) {
    char serial[16];
    char stateid[16];
    char ipbuf[32];
    char *argv[8] = {0};

    snprintf(serial, sizeof(serial), "%u", rand_r(seed));
    snprintf(stateid, sizeof(stateid), "0");
    snprintf(ipbuf, sizeof(ipbuf), "%s", sw_pretty_ip(g_ip_pool[idx]));

    argv[0] = serial;
    argv[1] = stateid;
    argv[2] = "DN";
    argv[3] = ipbuf;
    argv[4] = "";
    argv[5] = "";
    argv[6] = "";

    if (dispatch_uam_message(client, argv) == -1) {
        return -1;
    }

    return 0;
}

static int send_uam_misc(uam_client_ctx_t *client, int idx, unsigned int *seed, const char *op) {
    char serial[16];
    char stateid[16];
    char ipbuf[32];
    char retention[16];
    char pub_port[16];
    char *argv[8] = {0};

    snprintf(serial, sizeof(serial), "%u", rand_r(seed));
    snprintf(stateid, sizeof(stateid), "0");
    snprintf(ipbuf, sizeof(ipbuf), "%s", sw_pretty_ip(g_ip_pool[idx]));
    snprintf(retention, sizeof(retention), "%u", 60 + (rand_r(seed) % 600));
    snprintf(pub_port, sizeof(pub_port), "%u", 1024 + (rand_r(seed) % 8192));

    argv[0] = serial;
    argv[1] = stateid;
    argv[2] = (char *)op;
    argv[3] = ipbuf;

    if (strcmp(op, "RE") == 0) {
        argv[4] = retention;
    } else if (strcmp(op, "IP") == 0) {
        argv[4] = pub_port;
    }

    return dispatch_uam_message(client, argv);
}

static int preload_sessions(void) {
    uam_client_ctx_t client;
    unsigned int seed = (unsigned int)time(NULL);

    if (init_uam_client(&client) == -1) {
        return -1;
    }

    for (int i = 0; i < g_cfg.session_target; i++) {
        if (send_uam_up(&client, i, &seed) == -1) {
            destroy_uam_client(&client);
            fprintf(stderr, "preload failed at session %d\n", i);
            return -1;
        }
    }

    destroy_uam_client(&client);
    return 0;
}

static int verify_loaded_sessions(void) {
    for (int i = 0; i < g_cfg.session_target; i++) {
        int state = 0;
        uint64_t max_in = 0;
        uint64_t max_out = 0;

        if (sw_session_status_nonblock(g_ip_pool[i], &state, NULL, NULL, NULL, NULL, NULL) == -1) {
            fprintf(stderr, "missing session for %s after preload\n", sw_pretty_ip(g_ip_pool[i]));
            return -1;
        }

        if (state != SW_SESSION_ST_RELEASED) {
            fprintf(stderr, "unexpected session state %d for %s after preload\n",
                    state, sw_pretty_ip(g_ip_pool[i]));
            return -1;
        }

        if (sw_gauge_limits(g_ip_pool[i], &max_in, &max_out, NULL, NULL, NULL, NULL, 1) == -1) {
            fprintf(stderr, "missing gauge for %s after preload\n", sw_pretty_ip(g_ip_pool[i]));
            return -1;
        }

        if (!max_in || !max_out) {
            fprintf(stderr, "invalid gauge limits for %s after preload\n", sw_pretty_ip(g_ip_pool[i]));
            return -1;
        }
    }

    return 0;
}

static int verify_sampled_state(unsigned int *seed) {
    for (int i = 0; i < SAMPLE_VERIFY_BATCH; i++) {
        int idx = rand_r(seed) % g_cfg.session_target;
        int rc;
        int state = 0;
        char *filter_name = NULL;

        rc = sw_session_status(g_ip_pool[idx], &state, NULL, &filter_name, NULL, NULL, NULL);
        if (rc == -1) {
            fprintf(stderr, "sample status failed for %s\n", sw_pretty_ip(g_ip_pool[idx]));
            return -1;
        }
        if (state != SW_SESSION_ST_CAPTURED && state != SW_SESSION_ST_RELEASED) {
            fprintf(stderr, "unexpected session state %d for %s\n",
                    state, sw_pretty_ip(g_ip_pool[idx]));
            return -1;
        }
        if (!filter_name) {
            fprintf(stderr, "missing filter name for %s\n", sw_pretty_ip(g_ip_pool[idx]));
            return -1;
        }
    }

    return 0;
}

static int drain_sessions(void) {
    uam_client_ctx_t client;
    unsigned int seed = (unsigned int)time(NULL);

    if (init_uam_client(&client) == -1) {
        return -1;
    }

    for (int i = 0; i < g_cfg.session_target; i++) {
        if (send_uam_dn(&client, i, &seed) == -1) {
            destroy_uam_client(&client);
            fprintf(stderr, "drain failed at session %d\n", i);
            return -1;
        }
    }

    destroy_uam_client(&client);
    return 0;
}

static int current_phase(void) {
    int elapsed = (int)(time(NULL) - g_round_started);
    int phase = (elapsed * 4) / g_cfg.duration_sec;

    if (phase < 0) phase = 0;
    if (phase > 3) phase = 3;
    return phase;
}

static void *maintenance_thread(void *arg) {
    thread_ctx_t *ctx = (thread_ctx_t *)arg;
    const char *filter_dir = sw_config_get("filter-dir", SW_FILTER_DIR);

    while (__atomic_load_n(&g_running, __ATOMIC_SEQ_CST)) {
        time_t now = time(NULL);
        sw_lhstat_do(now);
        sw_gauge_expire();
        sw_session_expire();
        sw_acct_commit(0);
        sw_filter_reload((char *)filter_dir, now);
        bump_progress(ctx->progress_idx);
        usleep(1000);
    }

    return NULL;
}

static void *uam_worker_thread(void *arg) {
    thread_ctx_t *ctx = (thread_ctx_t *)arg;
    unsigned int seed = (unsigned int)(time(NULL) ^ (ctx->thread_id * 2654435761u));
    uam_client_ctx_t client;
    int consecutive_failures = 0;

    if (init_uam_client(&client) == -1) {
        set_failed("failed to initialize UAM worker client");
        return NULL;
    }

    while (__atomic_load_n(&g_running, __ATOMIC_SEQ_CST)) {
        int idx = rand_r(&seed) % g_cfg.session_target;
        int phase = current_phase();
        int rc = 0;
        int active = 0;
        int state = 0;

        if (sw_session_status_nonblock(g_ip_pool[idx], &state, NULL, NULL, NULL, NULL, NULL) == 0 &&
            state == SW_SESSION_ST_RELEASED) {
            active = 1;
        }

        switch (phase) {
        case 0:
            if (!active) rc = send_uam_up(&client, idx, &seed);
            else if ((rand_r(&seed) % 3) == 0) rc = send_uam_misc(&client, idx, &seed, "ST");
            else rc = send_uam_misc(&client, idx, &seed, "LI");
            break;
        case 1:
            if ((rand_r(&seed) % 100) < 45 && active) rc = send_uam_dn(&client, idx, &seed);
            else if ((rand_r(&seed) % 100) < 90) rc = send_uam_up(&client, idx, &seed);
            else rc = send_uam_misc(&client, idx, &seed, "CN");
            break;
        case 2:
            switch (rand_r(&seed) % 4) {
            case 0: rc = send_uam_misc(&client, idx, &seed, "ST"); break;
            case 1: rc = send_uam_misc(&client, idx, &seed, "LI"); break;
            case 2: rc = send_uam_misc(&client, idx, &seed, "CN"); break;
            case 3: rc = send_uam_misc(&client, idx, &seed, "PE"); break;
            }
            break;
        default:
            switch (rand_r(&seed) % 6) {
            case 0: rc = send_uam_up(&client, idx, &seed); break;
            case 1: rc = active ? send_uam_dn(&client, idx, &seed) : send_uam_misc(&client, idx, &seed, "ST"); break;
            case 2: rc = send_uam_misc(&client, idx, &seed, "ST"); break;
            case 3: rc = send_uam_misc(&client, idx, &seed, "LI"); break;
            case 4: rc = send_uam_misc(&client, idx, &seed, "CN"); break;
            case 5: rc = send_uam_misc(&client, idx, &seed, "RE"); break;
            }
            break;
        }

        if (rc == -1) {
            if (++consecutive_failures >= 8) {
                set_failed("uam worker request failed repeatedly");
                break;
            }
            usleep(1000);
            continue;
        }

        consecutive_failures = 0;
        bump_progress(ctx->progress_idx);
    }

    destroy_uam_client(&client);
    return NULL;
}

static void *traffic_worker_thread(void *arg) {
    thread_ctx_t *ctx = (thread_ctx_t *)arg;
    unsigned int seed = (unsigned int)(time(NULL) ^ (ctx->thread_id * 40503u));
    int consecutive_failures = 0;

    while (__atomic_load_n(&g_running, __ATOMIC_SEQ_CST)) {
        int idx = rand_r(&seed) % g_cfg.session_target;
        int state = 0;

        if (sw_session_status_nonblock(g_ip_pool[idx], &state, NULL, NULL, NULL, NULL, NULL) == 0 &&
            state == SW_SESSION_ST_RELEASED) {
            if (replay_single_flow(g_ip_pool[idx], &seed) == -1) {
                if (++consecutive_failures >= 8) {
                    set_failed("traffic replay failed repeatedly");
                    break;
                }
                usleep(1000);
                continue;
            }
            consecutive_failures = 0;
        }

        bump_progress(ctx->progress_idx);
    }

    return NULL;
}

static int run_single_round(const char *config_path, const char *log_path) {
    pthread_t *uam_threads = NULL;
    pthread_t *traffic_threads = NULL;
    thread_ctx_t *thread_ctx = NULL;
    pthread_t maintenance;
    unsigned long last_total = 0;
    time_t last_change = time(NULL);
    unsigned int verify_seed = (unsigned int)time(NULL);
    int rc = 1;

    reset_round_state();

    if (setup_environment(config_path, log_path) == -1) {
        return 1;
    }

    if (preload_sessions() == -1 || verify_loaded_sessions() == -1) {
        cleanup_environment();
        return 1;
    }

    uam_threads = calloc((size_t)g_cfg.uam_threads, sizeof(*uam_threads));
    traffic_threads = calloc((size_t)g_cfg.traffic_threads, sizeof(*traffic_threads));
    thread_ctx = calloc((size_t)(g_cfg.uam_threads + g_cfg.traffic_threads + 1), sizeof(*thread_ctx));
    if (!uam_threads || !traffic_threads || !thread_ctx) {
        fprintf(stderr, "failed to allocate thread descriptors\n");
        goto out;
    }

    g_round_started = time(NULL);

    for (int i = 0; i < g_cfg.uam_threads; i++) {
        thread_ctx[i].progress_idx = i;
        thread_ctx[i].thread_id = i;
        if (pthread_create(&uam_threads[i], NULL, uam_worker_thread, &thread_ctx[i]) != 0) {
            set_failed("failed to create uam worker");
            goto stop;
        }
    }

    for (int i = 0; i < g_cfg.traffic_threads; i++) {
        int slot = g_cfg.uam_threads + i;
        thread_ctx[slot].progress_idx = slot;
        thread_ctx[slot].thread_id = i;
        if (pthread_create(&traffic_threads[i], NULL, traffic_worker_thread, &thread_ctx[slot]) != 0) {
            set_failed("failed to create traffic worker");
            goto stop;
        }
    }

    thread_ctx[g_cfg.uam_threads + g_cfg.traffic_threads].progress_idx = g_cfg.uam_threads + g_cfg.traffic_threads;
    thread_ctx[g_cfg.uam_threads + g_cfg.traffic_threads].thread_id = 0;
    if (pthread_create(&maintenance, NULL, maintenance_thread,
                       &thread_ctx[g_cfg.uam_threads + g_cfg.traffic_threads]) != 0) {
        set_failed("failed to create maintenance worker");
        goto stop;
    }

    for (int elapsed = 0;
         elapsed < g_cfg.duration_sec && !__atomic_load_n(&g_failed, __ATOMIC_SEQ_CST);
         elapsed++) {
        int slots = g_cfg.uam_threads + g_cfg.traffic_threads + 1;
        unsigned long total = 0;

        sleep(1);
        for (int i = 0; i < slots; i++) {
            total += load_progress(i);
        }

        if (verify_sampled_state(&verify_seed) == -1) {
            set_failed("sample verification failed");
            break;
        }

        if (total != last_total) {
            last_total = total;
            last_change = time(NULL);
        } else if (time(NULL) - last_change >= PROGRESS_STALL_TIMEOUT_SEC) {
            set_failed("progress stalled, possible deadlock");
            break;
        }
    }

stop:
    __atomic_store_n(&g_running, 0, __ATOMIC_SEQ_CST);
    global_alive_flag = 0;

    for (int i = 0; i < g_cfg.uam_threads; i++) {
        if (uam_threads && uam_threads[i] && timed_join(uam_threads[i], "uam worker") != 0) {
            __atomic_store_n(&g_failed, 1, __ATOMIC_SEQ_CST);
        }
    }

    for (int i = 0; i < g_cfg.traffic_threads; i++) {
        if (traffic_threads && traffic_threads[i] && timed_join(traffic_threads[i], "traffic worker") != 0) {
            __atomic_store_n(&g_failed, 1, __ATOMIC_SEQ_CST);
        }
    }

    if (thread_ctx && timed_join(maintenance, "maintenance worker") != 0) {
        __atomic_store_n(&g_failed, 1, __ATOMIC_SEQ_CST);
    }

    if (drain_sessions() == -1) {
        __atomic_store_n(&g_failed, 1, __ATOMIC_SEQ_CST);
    }

    sw_acct_commit(1);

    if (!__atomic_load_n(&g_failed, __ATOMIC_SEQ_CST)) {
        rc = 0;
    }

out:
    cleanup_environment();
    free(uam_threads);
    free(traffic_threads);
    free(thread_ctx);
    return rc;
}

int main(int argc, char **argv) {
    int passed = 0;
    int failed = 0;
    char config_path[4096];
    char log_path[4096];
    char detail_path[4096];
    char filter_path[4096];
    char filter_file_path[4096];

    if (parse_args(argc, argv) == -1) {
        return 1;
    }

    g_progress = calloc((size_t)(g_cfg.uam_threads + g_cfg.traffic_threads + 1), sizeof(*g_progress));
    if (!g_progress) {
        fprintf(stderr, "failed to allocate progress array\n");
        return 1;
    }

    if (build_paths(config_path, sizeof(config_path),
                    log_path, sizeof(log_path),
                    detail_path, sizeof(detail_path),
                    filter_path, sizeof(filter_path),
                    filter_file_path, sizeof(filter_file_path)) == -1) {
        free((void *)g_progress);
        return 1;
    }

    if (prepare_artifacts(config_path, detail_path, filter_path, filter_file_path) == -1) {
        free((void *)g_progress);
        return 1;
    }

    for (int round = 1; round <= g_cfg.rounds; round++) {
        int rc = run_single_round(config_path, log_path);
        if (rc == 0) {
            passed++;
            printf("round %d/%d: passed\n", round, g_cfg.rounds);
        } else {
            failed++;
            printf("round %d/%d: failed\n", round, g_cfg.rounds);
        }
    }

    unlink(config_path);
    printf("coreTest_new summary: passed=%d failed=%d rounds=%d duration=%d sessions=%d uam_threads=%d traffic_threads=%d\n",
           passed, failed, g_cfg.rounds, g_cfg.duration_sec,
           g_cfg.session_target, g_cfg.uam_threads, g_cfg.traffic_threads);

    free((void *)g_progress);
    return failed == 0 ? 0 : 1;
}
