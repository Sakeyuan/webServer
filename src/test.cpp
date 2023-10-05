#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <iostream>
#include "./Config/Config.h"
using namespace std;
int main()
{
    Config* config = Config::getInstance();
    cout << config->getSqlHost() << endl;
    return 0;
}