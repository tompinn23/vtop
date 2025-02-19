#include "vtop.h"

#include "log.h"

#include <stdlib.h>
#include <sys/poll.h>
#include <signal.h>

volatile sig_atomic_t hup = 0;
volatile sig_atomic_t child = 0;
volatile sig_atomic_t term = 0;

static void sighandleterm(int sig) {
    term = 1;
}

static void sighandlechild(int sig) {
    child = 1;
}

void run_loop(int sv) {
    struct pollfd *pfds;
    sigset_t sigset;
    struct sigaction sa;
    int ret;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    sa.sa_handler = sighandleterm;
    if(sigaction(SIGTERM, &sa, NULL) == -1) {
      un_log_errno(LOG_ERR, "signal: set TERM");
      goto out;
    }
    sa.sa_handler = sighandlechild;
    if(sigaction(SIGCHLD, &sa, NULL) == -1) {
      un_log_errno(LOG_ERR, "signal: set CHILD");
      goto out;
    }
  
    
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGCHLD);

out:
    return -1;
}