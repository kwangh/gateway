/*
 * monitor.cc
 *
 *  Created on: 2019. 8. 26.
 *      Author: kwanghun_choi@tmax.co.kr
 */

#include <iostream>
#include <boost/asio.hpp>

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <inttypes.h>

static inline void __auto_free__(void *p)
{
  free(*(void **) p);
}

#define __do_free __attribute__((__cleanup__(__auto_free__)))
#define FNV1A_64_INIT ((uint64_t)0xcbf29ce484222325ULL)

typedef enum
{
  lxc_msg_state, lxc_msg_priority, lxc_msg_exit_code,
} lxc_msg_type_t;

struct lxc_msg
{
  lxc_msg_type_t type;
  char name[NAME_MAX + 1];
  int value;
};

typedef enum
{
  STOPPED, STARTING, RUNNING, STOPPING, ABORTING, FREEZING, FROZEN, THAWED, MAX_STATE,
} lxc_state_t;

static const char * const strstate[] = { "STOPPED", "STARTING", "RUNNING", "STOPPING", "ABORTING", "FREEZING",
    "FROZEN", "THAWED", };

const char *lxc_state2str(lxc_state_t state)
{
  if (state < STOPPED || state > MAX_STATE - 1)
    return NULL;
  return strstate[state];
}

ssize_t lxc_read_nointr(int fd, void *buf, size_t count)
{
  ssize_t ret;
  again: ret = read(fd, buf, count);
  if (ret < 0 && errno == EINTR)
    goto again;

  return ret;
}

void *must_realloc(void *orig, size_t sz)
{
  void *ret;

  do
  {
    ret = realloc(orig, sz);
  } while (!ret);

  return ret;
}

uint64_t fnv_64a_buf(void *buf, size_t len, uint64_t hval)
{
  unsigned char *bp;

  for (bp = (unsigned char *) buf; bp < (unsigned char *) buf + len; bp++)
  {
    /* xor the bottom with the current octet */
    hval ^= (uint64_t) * bp;

    /* gcc optimised:
     * multiply by the 64 bit FNV magic prime mod 2^64
     */
    hval += (hval << 1) + (hval << 4) + (hval << 5) + (hval << 7) + (hval << 8) + (hval << 40);
  }

  return hval;
}

int lxc_monitor_sock_name(const char *lxcpath, struct sockaddr_un *addr)
{
  __do_free char *path = NULL;
  size_t len;
  int ret;
  uint64_t hash;

  /* addr.sun_path is only 108 bytes, so we hash the full name and
   * then append as much of the name as we can fit.
   */
  memset(addr, 0, sizeof(*addr));
  addr->sun_family = AF_UNIX;

  /* strlen("lxc/") + strlen("/monitor-sock") + 1 = 18 */
  len = strlen(lxcpath) + 18;
  path = (char *) must_realloc(NULL, len);
  ret = snprintf(path, len, "lxc/%s/monitor-sock", lxcpath);
  if (ret < 0 || (size_t) ret >= len)
  {
    printf("Failed to create name for monitor socket\n");
    return -1;
  }

  /* Note: snprintf() will \0-terminate addr->sun_path on the 106th byte
   * and so the abstract socket name has 105 "meaningful" characters. This
   * is absolutely intentional. For further info read the comment for this
   * function above!
   */
  len = sizeof(addr->sun_path) - 1;
  hash = fnv_64a_buf(path, ret, FNV1A_64_INIT);
  ret = snprintf(addr->sun_path, len, "@lxc/%016" PRIx64 "/%s", hash, lxcpath);
  if (ret < 0)
  {
    printf("Failed to create hashed name for monitor socket\n");
    goto on_error;
  }
  else if ((size_t) ret >= len)
  {
    errno = ENAMETOOLONG;
    printf("The name of monitor socket too long (%d bytes)\n", ret);
    goto on_error;
  }

  /* replace @ with \0 */
  addr->sun_path[0] = '\0';
  printf("Using monitor socket name \"%s\" (length of socket name %zu must be <= %zu)\n", &addr->sun_path[1],
      strlen(&addr->sun_path[1]), sizeof(addr->sun_path) - 3);

  return 0;

  on_error: return -1;
}

static ssize_t lxc_abstract_unix_set_sockaddr(struct sockaddr_un *addr, const char *path)
{
  size_t len;

  if (!addr || !path)
  {
    errno = EINVAL;
    return -1;
  }

  /* Clear address structure */
  memset(addr, 0, sizeof(*addr));

  addr->sun_family = AF_UNIX;

  len = strlen(&path[1]);

  /* do not enforce \0-termination */
  if (len >= INT_MAX || len >= sizeof(addr->sun_path))
  {
    errno = ENAMETOOLONG;
    return -1;
  }

  /* do not enforce \0-termination */
  memcpy(&addr->sun_path[1], &path[1], len);
  return len;
}

int lxc_abstract_unix_connect(const char *path)
{
  int fd, ret;
  ssize_t len;
  struct sockaddr_un addr;

  fd = socket(PF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (fd < 0)
    return -1;

  len = lxc_abstract_unix_set_sockaddr(&addr, path);
  if (len < 0)
  {
    int saved_errno = errno;
    close(fd);
    errno = saved_errno;
    return -1;
  }

  ret = connect(fd, (struct sockaddr *)&addr,
      offsetof(struct sockaddr_un, sun_path) + len + 1);
  if (ret < 0)
  {
    int saved_errno = errno;
    close(fd);
    errno = saved_errno;
    return -1;
  }

  return fd;
}

void handler(const boost::system::error_code& error,std::size_t bytes_transferred){

};

int main()
{
  pid_t pid;

  if (pid = fork())
  {
    sleep(2);
    if (pid > 0)
    {
      kill(pid, SIGKILL);
    }

    struct sockaddr_un addr;
    int fd;
    size_t retry;
    int backoff_ms[] = { 10, 50, 100 };
    addr.sun_family = AF_UNIX;
    char *path = NULL;

    const char *lxcpath = "/var/lib/lxc";

    if (lxc_monitor_sock_name(lxcpath, &addr) < 0)
      return -1;

    printf("Opening monitor socket %s with len %zu\n", &addr.sun_path[1], strlen(&addr.sun_path[1]));

    for (retry = 0; retry < sizeof(backoff_ms) / sizeof(backoff_ms[0]); retry++)
    {
      fd = lxc_abstract_unix_connect(addr.sun_path);
      if (fd != -1 || errno != ECONNREFUSED)
        break;

      printf("Failed to connect to monitor socket. Retrying in %d ms\n", backoff_ms[retry]);
      usleep(backoff_ms[retry] * 1000);
    }

    if (fd < 0)
    {
      printf("Failed to connect to monitor socket\n");
      return -1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;

    struct lxc_msg msglxc;
    int ret;
    for (;;)
    {
      ret = lxc_read_nointr(fd, &msglxc, sizeof(msglxc));
      if (ret != sizeof(msglxc))
      {
        std::cerr << "Reading from fifo failed\n";
        close(fd);
        return -1;
      }
      else
      {
        msglxc.name[sizeof(msglxc.name) - 1] = '\0';

        switch (msglxc.type)
        {
          case lxc_msg_state:
            printf("'%s' changed state to [%s]\n", msglxc.name, lxc_state2str((lxc_state_t) msglxc.value));
            break;
          case lxc_msg_exit_code:
            printf("'%s' exited with status [%d]\n", msglxc.name, WEXITSTATUS((lxc_state_t) msglxc.value));
            break;
          default:
            std::cout << msglxc.type << std::endl;
            break;
        }
      }
    }
  }
  else
  {
    std::cout << "자식 프로세스에서 lxc-monitor 구동 시작\n";
    if (execl("/usr/bin/lxc-monitor", "lxc-monitor", NULL) == -1)
    {
      std::cerr << "execl error\n";
      return 1;
    }

    std::cout << "Should not be seen\n";
    return 0;
  }

  return 0;
}

