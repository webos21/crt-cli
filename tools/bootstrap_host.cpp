// Temporary hosted seed; never distributed or used as a product launcher.
#include "../src/main/core.h"
int main(int argc, char** argv) {
    using namespace crtcli;
    try { return print_reply(workflow(std::vector<std::string>(argv + 1, argv + argc)), true); }
    catch (const std::exception& error) { return print_reply({1, "seed", {{"message", error.what()}}, error.what()}, true); }
}
