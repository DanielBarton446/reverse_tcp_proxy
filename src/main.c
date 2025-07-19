#include "manager.h"
#include <unistd.h>

int main(int argc, char* argv[]) {

    Manager* manager = manager_init(12345);
    run_manager(manager);

    return 0;
}
