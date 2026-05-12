#include "wifi_utils.h"

char *get_rssi_bars(int8_t rssi) {
  if (rssi >= -50) {
    return "\033[0;32m" // green
           "▂"
           "▄"
           "▆"
           "█"
           "\033[0m";
  } else if (rssi >= -65) {
    return "\033[0;33m" // yellow
           "▂"
           "▄"
           "▆"
           "\033[0;37m"
           "█"
           "\033[0m";
  } else if (rssi >= -85) {
    return "\033[0;33m" // yellow
           "▂"
           "▄"
           "\033[0;37m"
           "▆"
           "█"
           "\033[0m";
  } else {
    return "\033[0;31m" // red
           "▂"
           "\033[0;37m"
           "▄"
           "▆"
           "█"
           "\033[0m";
  }
}