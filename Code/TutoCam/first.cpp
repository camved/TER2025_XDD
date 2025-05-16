#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/app/Application.h>

using namespace elm;
using namespace otawa;

class First: public Application {
public:
    First(void): Application("first", Version(1, 0, 0)) { }

protected:
    void work(const string &entry, PropList &props) override {
        Address addr = workspace()->process()->findLabel(entry);
        cout << entry << " found at " << addr << io::endl;
    }

};

OTAWA_RUN(First)