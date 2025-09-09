// File: src/DynaRangeCli.cpp
#include "core/Analysis.hpp"
#include "core/Arguments.hpp"
#include "core/Engine.hpp"

#include <iostream>
#include <vector>
#include <string>

#include <libintl.h>
#include <locale.h>

#define _(string) gettext(string)

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "");
    bindtextdomain("dynrange", "locale");
    textdomain("dynrange");

    // 1. Parse arguments
    ProgramOptions opts = ParseArguments(argc, argv);
    
    // 2. Call the engine with the console (std::cout) as the log stream
    if (!RunDynamicRangeAnalysis(opts, std::cout)) {
        std::cerr << _("A critical error occurred during processing. Please check the log.") << std::endl;
        return 1;
    }
    
    return 0;
}