#include "ping.h"
#include <esp_log.h>
#include <stdbool.h>

#define PING_INTERVAL_MS CONFIG_PING_INTERVAL_MS
#define PING_TIMEOUT_MS CONFIG_PING_TIMEOUT_MS

int failures;
bool network_state, last_network_state;

char *PING_TAG = "PING";

typedef struct {
  void (*func)(bool state, int bit);
  int has_internet_bit;
} network_change_t;

void network_state_change_check(network_change_t *network_change, bool state) {
  network_change->func(state, network_change->has_internet_bit);
}

static void cmd_ping_on_ping_success(esp_ping_handle_t hdl, void *args) {
  uint8_t ttl;
  uint16_t seqno;
  uint32_t elapsed_time, recv_len;
  ip_addr_t target_addr;
  failures = 0;
  esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
  esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
  esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr,
                       sizeof(target_addr));
  esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
  esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time,
                       sizeof(elapsed_time));
  ESP_LOGD(PING_TAG,
           "%" PRIu32 " bytes from %s icmp_seq=%" PRIu16 " ttl=%" PRIu16
           " time=%" PRIu32 " ms",
           recv_len, ipaddr_ntoa((ip_addr_t *)&target_addr), seqno, ttl,
           elapsed_time);

  network_state_change_check(args, true);
}

static void cmd_ping_on_ping_timeout(esp_ping_handle_t hdl, void *args) {
  failures++;
  uint16_t seqno;
  ip_addr_t target_addr;
  esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
  esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr,
                       sizeof(target_addr));
  ESP_LOGD(PING_TAG, "From %s icmp_seq=%d timeout",
           ipaddr_ntoa((ip_addr_t *)&target_addr), seqno);
  network_state_change_check(args, false);
}

static void cmd_ping_on_ping_end(esp_ping_handle_t hdl, void *args) {
  ip_addr_t target_addr;
  uint32_t transmitted;
  uint32_t received;
  uint32_t total_time_ms;
  esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted,
                       sizeof(transmitted));
  esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
  esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr,
                       sizeof(target_addr));
  esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms,
                       sizeof(total_time_ms));
  uint32_t loss = (uint32_t)((1 - ((float)received) / transmitted) * 100);
  if (IP_IS_V4(&target_addr)) {
    printf("\n--- %s ping statistics ---\n",
           inet_ntoa(*ip_2_ip4(&target_addr)));
  } else {
    printf("\n--- %s ping statistics ---\n",
           inet6_ntoa(*ip_2_ip6(&target_addr)));
  }
  printf("%" PRIu32 " packets transmitted, %" PRIu32 " received, %" PRIu32
         "%% packet loss, time %" PRIu32 "ms\n",
         transmitted, received, loss, total_time_ms);
  // delete the ping sessions, so that we clean up all resources and can create
  // a new ping session we don't have to call delete function in the callback,
  // instead we can call delete function from other tasks
  esp_ping_delete_session(hdl);
}

esp_ping_handle_t do_ping_cmd(char *address, int netif_index,
                              int has_internet_bit,
                              void (*fun_ptr)(bool state, int bit)) {
  esp_ping_handle_t *ping = malloc(sizeof(esp_ping_handle_t));
  esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();

  config.interface = netif_index;
  // Default network state is false
  last_network_state = false;
  network_state = false;

  config.count = 0;
  config.interval_ms = PING_INTERVAL_MS;
  config.timeout_ms = PING_TIMEOUT_MS;

  // parse IP address
  struct sockaddr_in6 sock_addr6;
  ip_addr_t target_addr;
  memset(&target_addr, 0, sizeof(target_addr));

  if (inet_pton(AF_INET6, address, &sock_addr6.sin6_addr) == 1) {
    /* convert ip6 string to ip6 address */
    ipaddr_aton(address, &target_addr);
  } else {
    struct addrinfo hint;
    struct addrinfo *res = NULL;
    memset(&hint, 0, sizeof(hint));
    /* convert ip4 string or hostname to ip4 or ip6 address */
    if (getaddrinfo(address, NULL, &hint, &res) != 0) {
      printf("ping: unknown host %s\n", address);
    }
    if (res->ai_family == AF_INET) {
      struct in_addr addr4 = ((struct sockaddr_in *)(res->ai_addr))->sin_addr;
      inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
    } else {
      struct in6_addr addr6 =
          ((struct sockaddr_in6 *)(res->ai_addr))->sin6_addr;
      inet6_addr_to_ip6addr(ip_2_ip6(&target_addr), &addr6);
    }
    freeaddrinfo(res);
  }
  config.target_addr = target_addr;

  /* set callback functions */
  network_change_t *network_change =
      malloc(sizeof(network_change_t)); /// Need to deallocate
  network_change->func = fun_ptr;
  network_change->has_internet_bit = has_internet_bit;

  esp_ping_callbacks_t cbs = {
      .cb_args = network_change,
      .on_ping_success = cmd_ping_on_ping_success,
      .on_ping_timeout = cmd_ping_on_ping_timeout,
      .on_ping_end = cmd_ping_on_ping_end,
  };

  esp_ping_new_session(&config, &cbs, ping);
  esp_ping_start(*ping);
  return ping;
}

void delete_ping_cmd(esp_ping_handle_t *ping) {
  esp_ping_stop(*ping);
  esp_ping_delete_session(*ping);
  free(ping);
}