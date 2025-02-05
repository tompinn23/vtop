#if defined(XEV_ENABLE_EPOLL)
X(EPOLL)
#endif
#if defined(XEV_ENABLE_POLL)
X(POLL)
#endif
#if defined(XEV_ENABLE_IOCP)
X(IOCP)
#endif
#if defined(XEV_ENABLE_URING)
X(URING)
#endif