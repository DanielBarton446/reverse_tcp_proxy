#include "manager.h"
#include <unistd.h>

int main(int argc, char* argv[]) {

    Manager* manager = manager_init(12345);
    run_manager(manager);
    argv[0] = "FOOBAR";

    return 0;
}
