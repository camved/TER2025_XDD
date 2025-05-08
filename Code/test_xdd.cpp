#include </home/camille/otawa/include/otawa/xdd/XddManager.hpp>

using namespace otawa::xengine;

int main() {
    StandardXddManager man;

    auto x = Xdd(&man.opsMan(), man.nMan().mK(3)); // variable 3
    auto y = Xdd(&man.opsMan(), man.nMan().mK(1)); // variable 1
    auto z = x + y; // union logique

    return 0;
}
