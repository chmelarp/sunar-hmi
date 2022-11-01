// 
// File:   test.cc
// Author: chmelarp, xgupta00
//
// Created on June 2, 2008, 1:54 PM
//

#define _DEBUG

#include "abbrevs.h"
#include "cvffmpeg.h"

#include <string.h>
#include <iostream>
#include <stdlib.h>


// assign null to the new media!
ffMedia* media = null;

/**
 *  Prints the error to the cerr and exits nicely
 */
void error(String message) {
    cerr << "Error (main): " << message << endl;

    // close all the stuff
    ffClose(media);

    exit(1);
}


/** ****************************************************************************
 * Main
 */ 
int main(int argc, char** argv)
{
    // allocate new media (or assign null!)
    media = newMedia();
    
    // temp. char buffer for itoa. - do not use 1 buf multiple times per command
    char* buf = alloc(char, 32); 
    char c = 0;
    
    // Usage: 
    if (argc < 2) {
        error("Usage: cvffmpeg video1.avi ... video219.mpeg");
    }

    #ifdef _DEBUG
        cvNamedWindow("cvffmpeg", 0);    
        CvFont      font;
        cvInitFont( &font, CV_FONT_HERSHEY_PLAIN, 1.0, 0.8, 0, 1, 8 ); // or CV_AA instead of 8 and anti-alias...
    #endif

    // process argv and load (videa) /////////////////////////////////////////////////////////////////
    int argNo = 1;
    while (argc > argNo) { 
        // experiment start time (without preparation)
        long start_time = time(NULL); //  / CLOCKS_PER_SEC
        
        // open the media file
        if ( !ffOpenFile(media, argv[argNo]) ) {
            error("Couldn't open file " + String(argv[argNo]));
        }

        // create a directory
        system((String("mkdir ") + argv[argNo] + "_kf/").c_str());
        
        // extract all images (current step is 50 frames)
        do {
            // get the next frame
            ffCvFramePreciseSeek(media, ffPosition(media) + 50);
            if (media->ffEof) break;            

            // show the image... 
            #ifdef _DEBUG
                // print frame number
                sprintf(buf, "%lu", ffPosition(media));
                cvPutText( media->sws->cvFrame, buf, cvPoint(10,20), &font, cvRed);
                cvShowImage("cvffmpeg", media->sws->cvFrame);
                c = cvWaitKey(5);  // because of bs
            #endif        
            
            // file name
            sprintf(buf, "%lu", ffPosition(media));
            String fileSave = String(argv[argNo]) + "_kf/kf_" + buf + ".jpg";
            
            // save image
            cvSaveImage(fileSave.c_str(), media->sws->cvFrame);
            
        } while (c != 'q');   // or just true     
    
        // report the processing time
        cout << endl << "Video " << argv[argNo] << " run time: " << (double)(time(NULL) - start_time) << endl << endl;
        
        // close the video
        ffClose(media);
        
        // process other argument
        argNo++;
    } // while argc
    
    
    if (buf != null) free(buf);

    #ifdef _DEBUG    
        //  cvWaitKey();
    #endif
    
    return (EXIT_SUCCESS);
}
