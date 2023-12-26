#pragma once

#include "master.h"

struct Clients
{
    set<int> socks;
    map<int, string> nicknames;
};

int init_listener_socket();
void serve(int listener, Clients &clients, vector<int> &socks_to_erase, timeval &timeout);
