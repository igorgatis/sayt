#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <libgen.h>
#include <libc/cosmo.h>
#include <libc/dce.h>
#include <libc/calls/calls.h>
#include <libc/str/str.h>
#include <libc/stdio/append.h>
#include <net/https/fetch.h>

#define SAYT_VERSION_ENV "SAYT_VERSION"
#define SAYT_CLI_DEBUG_ENV "SAYT_CLI_DEBUG"
// TODO(igor.gatis): replace with final location.
#define SAYT_MISE_LOCATION "github:igorgatis/sayt"
#ifndef SAYT_BUILD_VERSION
#define SAYT_BUILD_VERSION "dev"
#endif

#define MISE_VERSION "v2025.11.11"
#define MISE_RELEASES_URL "https://github.com/jdx/mise/releases/download"
#define COSMOS_BIN_URL "https://cosmo.zip/pub/cosmos/v/4.0.2/bin"

#define MAX_ARGV_LEN 64

#define EMBEDDED_CA_CERTS_DIR "/zip/usr/share/ssl/root"
#define CA_CERTS_FILE "ca-certificates.crt"
#define EMBEDDED_CA_CERTS_FILE EMBEDDED_CA_CERTS_DIR "/" CA_CERTS_FILE

#define debugf(fmt, ...) do { \
  if (getenv(SAYT_CLI_DEBUG_ENV)) { \
    fprintf(stderr, "[sayt:%s] ", SAYT_BUILD_VERSION); \
    fprintf(stderr, fmt, ##__VA_ARGS__); \
  } \
} while(0)

#define debug_args(argv) do { \
  if (getenv(SAYT_CLI_DEBUG_ENV)) { \
    fprintf(stderr, "[sayt:%s] ", SAYT_BUILD_VERSION); \
    for (int _i = 0; (argv)[_i]; _i++) { \
      if (_i > 0) fprintf(stderr, " "); \
      fprintf(stderr, "%s", (argv)[_i]); \
    } \
    fprintf(stderr, "\n"); \
  } \
} while(0)

#define join_path(out, sep, ...) join_path_impl(out, sep, __VA_ARGS__, NULL)

void join_path_impl(char* out, const char* in_sep, ...) {
  va_list args;
  va_start(args, in_sep);
  out[0] = '\0';
  int first = 1;
  const char* part;
  while ((part = va_arg(args, const char*)) != NULL) {
    if (!first) {
      strlcat(out, in_sep, PATH_MAX);
    }
    strlcat(out, part, PATH_MAX);
    first = 0;
  }
  va_end(args);
}

typedef struct {
  char mise_dir[PATH_MAX];
  char mise_bin[PATH_MAX];
  char mise_url[PATH_MAX];
  char mise_pkg[PATH_MAX];

  char unzip_bin[PATH_MAX];

  bool sayt_installed;
  char sayt_version[PATH_MAX];
  char sayt_nu[PATH_MAX];
  char nu_toml[PATH_MAX];

  char sayt_at_version[PATH_MAX];
} Context;

const char* detect_os() {
  if (IsWindows()) return "windows";
  if (IsXnu()) return "macos";
  if (IsLinux()) return "linux";
  if (IsFreebsd()) return "linux";
  if (IsOpenbsd()) return "linux";
  if (IsNetbsd()) return "linux";
  return "unknown";
}

const char* detect_arch() {
  if (IsAarch64()) return "arm64";
  struct utsname uts;
  if (uname(&uts) == 0) {
    if (strcmp(uts.machine, "x86_64") == 0) return "x64";
    if (strcmp(uts.machine, "amd64") == 0) return "x64";
    if (strcmp(uts.machine, "aarch64") == 0) return "arm64";
    if (strcmp(uts.machine, "arm64") == 0) return "arm64";
    if (strcmp(uts.machine, "armv7l") == 0) return "armv7";
  }
  return "unknown";
}

char* sayt_bin_name() {
  // if (IsLinux()) {
  //   const char* arch = detect_arch();
  //   if (strcmp(arch, "x64") == 0) {
  //     return "sayt-amd64.elf";
  //   }
  //   return  "sayt-arm64.elf";
  // }
  return "sayt.com";
}

int file_exists(const char* in_path) {
  return access(in_path, F_OK) == 0;
}

int is_executable(const char* in_path) {
  return access(in_path, X_OK) == 0;
}

int copy_file(const char* src_path, const char* dst_path) {
  int src_fd = open(src_path, O_RDONLY);
  if (src_fd < 0) {
    fprintf(stderr, "Error: Failed to open %s: %s\n", src_path, strerror(errno));
    return -1;
  }

  int dst_fd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (dst_fd < 0) {
    close(src_fd);
    fprintf(stderr, "Error: Failed to create %s: %s\n", dst_path, strerror(errno));
    return -1;
  }

  char buf[8192];
  ssize_t n;
  while ((n = read(src_fd, buf, sizeof(buf))) > 0) {
    ssize_t written = write(dst_fd, buf, n);
    if (written != n) {
      close(src_fd);
      close(dst_fd);
      unlink(dst_path);
      fprintf(stderr, "Error: Failed to write %s: %s\n", dst_path, strerror(errno));
      return -1;
    }
  }

  close(src_fd);
  close(dst_fd);

  if (n < 0) {
    unlink(dst_path);
    fprintf(stderr, "Error: Failed to read %s: %s\n", src_path, strerror(errno));
    return -1;
  }

  return 0;
}

int write_file(const char* data, const char* dst_path) {
  int dst_fd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (dst_fd < 0) {
    fprintf(stderr, "Error: Failed to create %s: %s\n", dst_path, strerror(errno));
    return -1;
  }

  size_t len = strlen(data);
  ssize_t written = write(dst_fd, data, len);
  close(dst_fd);

  if (written != (ssize_t)len) {
    unlink(dst_path);
    fprintf(stderr, "Error: Failed to write %s: %s\n", dst_path, strerror(errno));
    return -1;
  }

  return 0;
}

int setup_linux_ssl_certs(const char* cache_dir) {
  // When EMBEDDED_CA_CERTS_DATA is defined, it means this is a regular
  // binary (not an APE binary) and thus EMBEDDED_CA_CERTS_FILE is missing.
#ifdef EMBEDDED_CA_CERTS_DATA
  debugf("Writing SSL embeded certs to %s\n", EMBEDDED_CA_CERTS_FILE);
  if (makedirs(EMBEDDED_CA_CERTS_DIR, 0755) != 0 && errno != EEXIST) {
    fprintf(stderr, "Error: Failed to create %s\n", EMBEDDED_CA_CERTS_DIR);
    return -1;
  }
  if (write_file(EMBEDDED_CA_CERTS_DATA, EMBEDDED_CA_CERTS_FILE) != 0) {
    return -1;
  }
#endif

  char cert_path[PATH_MAX];
  join_path(cert_path, "/", cache_dir, CA_CERTS_FILE);
  debugf("Copying SSL cert from %s to %s\n", EMBEDDED_CA_CERTS_FILE, cert_path);
  if (copy_file(EMBEDDED_CA_CERTS_FILE, cert_path) != 0) {
    return -1;
  }
  debugf("SSL_CERT_FILE=%s\n", cert_path);
  setenv("SSL_CERT_FILE", cert_path, 0);
  return 0;
}

void resolve_cache_dir(char* out_cache_dir) {
  const char* env;
  if (IsWindows()) {
    if ((env = getenv("LOCALAPPDATA"))) {
      join_path(out_cache_dir, "/", env, "sayt");
    } else if ((env = getenv("TEMP")) || (env = getenv("TMP"))) {
      join_path(out_cache_dir, "/", env, "sayt");
    } else {
      join_path(out_cache_dir, "/", "C", "Temp", "sayt");
    }
    return;
  }
  if (IsXnu()) {
    if ((env = getenv("HOME"))) {
      join_path(out_cache_dir, "/", env, "Library", "Caches", "sayt");
    } else {
      join_path(out_cache_dir, "/", "", "tmp", "sayt");
    }
    return;
  }
  if ((env = getenv("XDG_CACHE_HOME"))) {
    join_path(out_cache_dir, "/", env, "sayt");
  } else if ((env = getenv("HOME"))) {
    join_path(out_cache_dir, "/", env, ".cache", "sayt");
  } else {
    join_path(out_cache_dir, "/", "", "tmp", "sayt");
  }
}

int resolve_sayt_dir(const char* in_argv0, char* out_dir) {
  char resolved[PATH_MAX];
  if (realpath(in_argv0, resolved) == NULL) {
    strncpy(resolved, in_argv0, sizeof(resolved) - 1);
  }
  char* dir = dirname(resolved);
  char parent[PATH_MAX];
  join_path(parent, "/", dir, "..");
  char* resolved_parent = realpath(parent, NULL);
  if (resolved_parent) {
    strncpy(out_dir, resolved_parent, PATH_MAX - 1);
    free(resolved_parent);
    return 0;
  }
  fprintf(stderr, "Error: Could not resolve sayt directory\n");
  return -1;
}

int init_context(const char* in_argv0, Context* ctx) {
  memset(ctx, 0, sizeof(Context));

  char cache_dir[PATH_MAX];
  resolve_cache_dir(cache_dir);
  if (makedirs(cache_dir, 0755) != 0 && errno != EEXIST) {
    fprintf(stderr, "Error: Failed to create %s\n", cache_dir);
    return -1;
  }

  if (IsLinux()) {
    if (setup_linux_ssl_certs(cache_dir) != 0) {
      return -1;
    }
  }

  char mise_version_dir[64];
  snprintf(mise_version_dir, sizeof(mise_version_dir), "mise-%s", MISE_VERSION);
  join_path(ctx->mise_dir, "/", cache_dir, mise_version_dir);
  if (makedirs(ctx->mise_dir, 0755) != 0 && errno != EEXIST) {
    fprintf(stderr, "Error: Failed to create %s\n", ctx->mise_dir);
    return -1;
  }

  if (IsWindows()) {
    join_path(ctx->mise_bin, "/", ctx->mise_dir, "mise", "bin", "mise.exe");
    char pkg_name[256];
    snprintf(pkg_name, sizeof(pkg_name), "mise-%s-%s-%s.zip", MISE_VERSION, detect_os(), detect_arch());
    join_path(ctx->mise_url, "/", MISE_RELEASES_URL, MISE_VERSION, pkg_name);
    join_path(ctx->mise_pkg, "/", ctx->mise_dir, pkg_name);
    join_path(ctx->unzip_bin, "/", cache_dir, "unzip");
  } else {
    join_path(ctx->mise_bin, "/", ctx->mise_dir, "mise");
    char bin_name[256];
    const char* musl_suffix = IsLinux() ? "-musl" : "";
    snprintf(bin_name, sizeof(bin_name), "mise-%s-%s-%s%s",
             MISE_VERSION, detect_os(), detect_arch(), musl_suffix);
    join_path(ctx->mise_url, "/", MISE_RELEASES_URL, MISE_VERSION, bin_name);
  }

  char* version = getenv(SAYT_VERSION_ENV);
  if (!version) version = "latest";
  snprintf(ctx->sayt_at_version, PATH_MAX, "%s@%s", SAYT_MISE_LOCATION, version);

  char sayt_dir[PATH_MAX];
  if (resolve_sayt_dir(in_argv0, sayt_dir) != 0) {
    return -1;
  }
  join_path(ctx->sayt_version, "/", sayt_dir, ".version");
  ctx->sayt_installed = file_exists(ctx->sayt_version);
  join_path(ctx->sayt_nu, "/", sayt_dir, "sayt.nu");
  join_path(ctx->nu_toml, "/", sayt_dir, "nu.toml");

  debugf("context:\n");
  debugf("  cache_dir=%s\n", cache_dir);
  debugf("  mise_dir=%s\n", ctx->mise_dir);
  debugf("  mise_url=%s\n", ctx->mise_url);
  debugf("  mise_bin=%s\n", ctx->mise_bin);
  debugf("  sayt_at_version=%s\n", ctx->sayt_at_version);
  debugf("  sayt_installed=%s\n", ctx->sayt_installed ? "YES" : "NO");
  debugf("  sayt_dir=%s\n", sayt_dir);
  debugf("  sayt_nu=%s\n", ctx->sayt_nu);
  debugf("  nu_toml=%s\n", ctx->nu_toml);
  return 0;
}

int download_to_file(const char* in_url, const char* in_dest) {
  debugf("download_to_file %s -> %s\n", in_url, in_dest);
  char* data = NULL;
  int status = AppendFetch(&data, in_url);
  if (status < 0) {
    fprintf(stderr, "Error: Failed to fetch %s: %s\n", in_url, strerror(errno));
    free(data);
    return -1;
  }
  size_t len = data ? appendz(data).i : 0;
  debugf("download_to_file status=%d size=%zu\n", status, len);
  if (status < 200 || status >= 300 || !data) {
    fprintf(stderr, "Error: Failed to fetch %s: HTTP %d\n", in_url, status);
    free(data);
    return -1;
  }
  int fd = open(in_dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    fprintf(stderr, "Error: Failed to create %s: %s\n", in_dest, strerror(errno));
    free(data);
    return -1;
  }
  ssize_t written = write(fd, data, len);
  close(fd);
  free(data);
  if (written != (ssize_t)len) {
    fprintf(stderr, "Error: Failed to write %s: %s\n", in_dest, strerror(errno));
    return -1;
  }
  return 0;
}

int download_tool(const char* in_name, const char* in_dest) {
  if (is_executable(in_dest)) return 0;
  char url[PATH_MAX];
  join_path(url, "/", COSMOS_BIN_URL, in_name);
  if (download_to_file(url, in_dest) != 0) {
    return -1;
  }
  chmod(in_dest, 0755);
  return 0;
}

int run_cmd(char* const in_argv[]) {
  debug_args(in_argv);
  pid_t pid = fork();
  if (pid == -1) return -1;
  if (pid == 0) {
    execv(in_argv[0], (char* const*)in_argv);
    _exit(127);
  }
  int status;
  waitpid(pid, &status, 0);
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

int fetch_mise(Context* ctx) {
  if (is_executable(ctx->mise_bin)) {
    debugf("mise already exists at %s\n", ctx->mise_bin);
    return 0;
  }

  if (endswith(ctx->mise_pkg, ".zip")) {
    if (download_to_file(ctx->mise_url, ctx->mise_pkg) != 0) {
      fprintf(stderr, "Error: Failed to download mise from %s\n", ctx->mise_url);
      return -1;
    }
    debugf("Extracting mise %s\n", ctx->mise_pkg);
    if (download_tool("unzip", ctx->unzip_bin) != 0) return -1;
    char* unzip_cmd[] = {ctx->unzip_bin, "-q", ctx->mise_pkg, "-d", ctx->mise_dir, NULL};
    if (run_cmd(unzip_cmd) != 0) {
      fprintf(stderr, "Error: Failed to extract %s\n", ctx->mise_pkg);
      return -1;
    }
  } else {
    if (download_to_file(ctx->mise_url, ctx->mise_bin) != 0) {
      fprintf(stderr, "Error: Failed to download mise from %s\n", ctx->mise_url);
      return -1;
    }
    chmod(ctx->mise_bin, 0755);
  }

  if (!is_executable(ctx->mise_bin)) {
    fprintf(stderr, "Error: mise binary not found at %s\n", ctx->mise_bin);
    return -1;
  }

  return 0;
}

void append_argv(int* out_argc, char* out_argv[], char *const in_args[]) {
  for (int i = 0; in_args[i] && *out_argc < MAX_ARGV_LEN - 1; i++) {
    out_argv[(*out_argc)++] = in_args[i];
  }
  out_argv[*out_argc] = NULL;
}

int main(int argc, char* argv[]) {
  Context ctx;
  if (init_context(argv[0], &ctx) != 0) {
    return 1;
  }
  if (fetch_mise(&ctx) != 0) {
    return 1;
  }

  int new_argc = 0;
  char* new_argv[MAX_ARGV_LEN];

  if (ctx.sayt_installed) {
    debugf("Running %s\n", ctx.sayt_nu);
    append_argv(&new_argc, new_argv, (char *[]){
      ctx.mise_bin, "tool-stub", ctx.nu_toml, ctx.sayt_nu, NULL
    });
  } else {
    debugf("Bootstrapping %s\n", ctx.sayt_at_version);
    append_argv(&new_argc, new_argv, (char *[]){
      ctx.mise_bin, "exec", ctx.sayt_at_version, "--", sayt_bin_name(), NULL
    });
  }
  append_argv(&new_argc, new_argv, &argv[1]);

  debug_args(new_argv);
  extern char** environ;
  //execve(new_argv[0], new_argv, environ);
  systemvpe(new_argv[0], new_argv, environ);
  perror("execve failed");
  return 1;
}
