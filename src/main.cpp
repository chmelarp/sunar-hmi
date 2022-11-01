/* 
 * File:   main.cpp
 *
 * Author: Petr Chmelar (c) 2009
 * License: GNU/GPL and Intel CV
 *
 * Created on 05. 06. 2009, 17:23
 * Changed on
 */

// Include C/C++
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
// custom
#include "browse.h"

String actionStr = "exps";
       // minerva1 /mnt/minerva1/chmelarp/i-Lids/mct_tr
String videoLocation = String("../mct_tr"); //../2010/videos
String videoPrefix = String("./avss_");
String datasetStart = String("2");
String videoStart = String("1");
String trackStart = String("1");


/**
 * Exit with help
 */
void exitWithHelp() {
    cout << "Syntax: sunar-hmi -a annots [-p ./mc_tr] [-d 2] [-v 1]" << endl;
    cout << "        sunar-hmi -a handovers [-p ./mc_tr] [-d 2] [-v 1]" << endl;
    cout << "        sunar-hmi -a exps [-p ./mc_tr] [-t 1] [-o ./video_out_dir/prefix_]" << endl;
    exit(EXIT_FAILURE);
}

#define ifarg(nick,var) if (*i==(nick)){(var)=*++i;}
/**
 * Loop over command-line args
 */
void processArguments(int argc, char** argv) {
    if (argc <= 1) exitWithHelp();

    vector<string> args(argv + 1, argv + argc);
    for (vector<string>::iterator i = args.begin(); i != args.end(); ++i) {

        if (*i == "-h" || *i == "--help") {
            exitWithHelp();
        }
        else ifarg("-a", actionStr)
        else ifarg("-t", trackStart)
        else ifarg("-p", videoLocation)      // mc_tr... default ./
                else ifarg("-o", videoPrefix)      // mc_tr... default ./
        else ifarg("-d", datasetStart)
        else ifarg("-v", videoStart)
        else exitWithHelp();
    }
}

/**
 * Main fun
 * TODO: split annots and handovers according to the experiments: -a experiment -t track (internal identifier of experiments)
 * TODO: -o video_out.avi
 */
int main(int argc, char** argv) {

    // set locale to original POSIX / ANSI C
    setlocale (LC_ALL, "C");

    processArguments(argc, argv);
    if (actionStr.compare("annots") == 0) {
        browseAnnots(videoLocation, datasetStart, videoStart);
    }
    else if (actionStr.compare("handovers") == 0) {
        browseHandovers(videoLocation, datasetStart, videoStart);
   }
    else if (actionStr.compare("exps") == 0) {
        return browseExperiments(videoLocation, trackStart, videoPrefix);
    }

    // TODO: process other actions (if there are some)
    else {
        exitWithHelp();
    }

    return (EXIT_SUCCESS);
}
