#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <Windows.h>

#include "public.h"

int ServerMain();
int ClientMain();

void SendCmdToCient(int clientId, const char * cmd);
