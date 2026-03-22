#include "boot/boot_args.h"
#include <stddef.h>

static char g_boot_cmdline[1024] = {0};

void boot_args_init(const char *cmdline) {
  if (!cmdline) {
    return;
  }

  size_t i = 0;
  while (cmdline[i] != '\0' && i < sizeof(g_boot_cmdline) - 1) {
    g_boot_cmdline[i] = cmdline[i];
    i++;
  }
  g_boot_cmdline[i] = '\0';
}

const char *boot_get_cmdline(void) { return g_boot_cmdline; }

static int my_strlen(const char *str) {
  int len = 0;
  while (str[len] != '\0') {
    len++;
  }
  return len;
}

static int my_strncmp(const char *s1, const char *s2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (s1[i] != s2[i]) {
      return (unsigned char)s1[i] - (unsigned char)s2[i];
    }
    if (s1[i] == '\0') {
      return 0;
    }
  }
  return 0;
}

// Simple token finder: searches for `key` in `g_boot_cmdline`, returning
// pointer to its occurrence bounded by whitespace or null.
static const char *find_token(const char *key, size_t key_len,
                              bool require_equals) {
  const char *p = g_boot_cmdline;
  while (*p != '\0') {
    // Skip whitespace
    while (*p == ' ' || *p == '\t' || *p == '\n') {
      p++;
    }
    if (*p == '\0') {
      break;
    }

    const char *start = p;
    // Find end of token
    while (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\0') {
      p++;
    }
    size_t tok_len = p - start;

    if (tok_len >= key_len) {
      if (my_strncmp(start, key, key_len) == 0) {
        if (require_equals) {
          if (tok_len > key_len && start[key_len] == '=') {
            return start;
          }
        } else {
          if (tok_len == key_len || start[key_len] == '=') {
            return start;
          }
        }
      }
    }
  }
  return NULL;
}

bool boot_has_flag(const char *key) {
  if (!key || key[0] == '\0')
    return false;

  size_t key_len = my_strlen(key);
  return find_token(key, key_len, false) != NULL;
}

bool boot_get_kv(const char *key, char *out, size_t out_sz) {
  if (!key || key[0] == '\0' || !out || out_sz == 0)
    return false;

  size_t key_len = my_strlen(key);
  const char *token = find_token(key, key_len, true);
  if (!token) {
    return false;
  }

  const char *val_start = token + key_len + 1; // Skip "key="
  const char *p = val_start;

  while (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\0') {
    p++;
  }

  size_t val_len = p - val_start;
  if (val_len >= out_sz) {
    val_len = out_sz - 1;
  }

  for (size_t i = 0; i < val_len; i++) {
    out[i] = val_start[i];
  }
  out[val_len] = '\0';

  return true;
}
