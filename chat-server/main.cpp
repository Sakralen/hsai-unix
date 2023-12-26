#include "master.h"
#include "daemon.h"
#include "server.h"

int main(int argc, char *argv[])
{
    openlog(argv[0], LOG_PID, LOG_USER);

    daemonize();

    int listener = init_listener_socket();
    listen(listener, BACKLOG_SIZE);

    Clients clients;
    vector<int> socks_to_erase;

    timeval timeout = {TIMEOUT_SEC, 0};

    while (1)
    {
        serve(listener, clients, socks_to_erase, timeout);
    }
}
