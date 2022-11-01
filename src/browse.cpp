/*
 * File:   browseAnnots.cpp
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
// PostgreSQL
#include <postgresql/libpq-fe.h>
// custom
#include "cvffmpeg/abbrevs.h"
#include "cvffmpeg/cvffmpeg.h"
// headers
#include "browse.h"

// Assign null to the new media
ffMedia* media = null;


/**
 *
 */
void browseAnnots(String videoLocation, String datasetStart, String videoStart) {
    cout << "Preparing to start the Sunar HMI annotation presentation... " << endl;
    cout << "To skip the video, press 'w', to quit press 'q'." << endl;
    cout << "The video speed is controled by '1'..'5', 'space' is for steping." << endl << endl;

    // temp. char buffer for itoa, sprintf - do not use 1 buf multiple times per command
    char* buf = alloc(char, 32);
    char dc = '3';
    dc_delay = setSpeed(dc);

    // TODO: parse arguments
    conninfo = CONNINFO;

    // Make a connection to the database
    conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK) {
        pgError(conn, "Connection to database failed: " + String(PQerrorMessage(conn)));
    }

    // Eg. "select * from pg_database"
    String objectsQuery = String("SELECT antr.dataset, antr.object, antr.video, antr.firsts[1], antr.lasts[1], antr.camera, antr.track, vs.name \n") +
            " FROM ONLY sunar.annotation_tracks AS antr JOIN sunar.videos AS vs \n" +
            "      ON antr.dataset=vs.dataset AND antr.camera=vs.camera AND antr.video=vs.video \n" +
            " WHERE antr.dataset=" + datasetStart +" AND antr.video="+ videoStart +" \n";
            " ORDER BY dataset, object, video, firsts[1], lasts[1], camera, track \n";
        // results
    PGresult* objectsRes = PQexec(conn, objectsQuery.c_str());
    if (PQresultStatus(objectsRes) != PGRES_TUPLES_OK) {
        pgError(conn, "SELECT failed: " + String(PQerrorMessage(conn)));
    }


    // display a display
    cvNamedWindow("Sunar HMI", 0);
    CvFont font, boldFont, largeFont;
    cvInitFont(&font, CV_FONT_HERSHEY_PLAIN, 1.0, 1.2, 0, 1, 8); // or CV_AA instead of 8 and anti-alias...
    cvInitFont(&boldFont, CV_FONT_HERSHEY_PLAIN, 1.0, 1.2, 0, 2, 8);
    cvInitFont(&largeFont, CV_FONT_HERSHEY_PLAIN, 1.8, 2.1, 0, 2, 8);

    // rememver last video name
    string vnameLast = "";

    //
    // go through annotations (videos, tracks and states)
    //
    for (int oi = 0; oi < PQntuples(objectsRes); oi++) {
        char* adst = PQgetvalue(objectsRes, oi, 0);
        char* aobj = PQgetvalue(objectsRes, oi, 1);
        char* avid = PQgetvalue(objectsRes, oi, 2);
        char* afst = PQgetvalue(objectsRes, oi, 3);
        char* alst = PQgetvalue(objectsRes, oi, 4);
        char* acam = PQgetvalue(objectsRes, oi, 5);
        char* antr = PQgetvalue(objectsRes, oi, 6);
        string vname = string(videoLocation) + "/" + PQgetvalue(objectsRes, oi, 7);

        // show the video with the following bounds
        int afirst = (atoi(afst)-25 < 1) ? 1 : atoi(afst)-25; // 25 frames before first frame of the annotation or the first one
        int alast = atoi(alst)+25;  // doesnt matter its more than the video lasts...


        //
        // show the video
        // if the video has changed this time... open a new one
        if (vname.compare(vnameLast) != 0) {
            cout << "Loading a new video ... " << endl;

            vnameLast = vname;

            // allocate new media (or assign null!)
            media = newMedia();
            // open the media file
            if ( !ffOpenFile(media, vname.c_str()) ) {
                error(string("Couldn't open file ") + vname);
            }

            // print seeking...
            cout << "Sunar is seeking ... " << endl;
            // draw the upcomming video information
            IplImage* image = cvCloneImage(media->sws->cvFrame);
            if (image->imageData == null) cvCreateImageData(image);
            string info = string("| Sunar | dataset ")+ adst +" | video "+ avid +" | camera "+ acam +" |";
            cvRectangle(image, cvPoint(0, 0), cvPoint(image->width-1, image->height-1), cvBlack, CV_FILLED, CV_AA);
            cvPutText(image, info.c_str(), cvPoint(10,20), &font, cvYellow);
            cvPutText(image, "Sunar is loading a new video ..." , cvPoint(10,200), &largeFont, cvWhite);
            cvShowImage("Sunar HMI", image);
            cvWaitKey(dc_delay);  // because of bs
            cvReleaseImage(&image);

            // seek to the new video position
            ffCvFramePreciseSeek(media, afirst-1);
            if (media->ffEof) continue;
        }


        //
        // select annotation_states data
        // and init positions and frames
        cout << "Loading annotations ... " << endl;
        string annotsQuery = string("SELECT anst.time, anst.position[0]::integer, anst.position[1]::integer, anst.size[0]::integer, anst.size[1]::integer, anst.occlusion\n") +
            " FROM ONLY sunar.annotation_states anst\n" +
            " WHERE anst.dataset="+ adst +" AND anst.video="+ avid +" AND anst.track="+ antr +" AND anst.camera="+ acam +"\n" +
            " ORDER BY time";
        PGresult* annotsRes = PQexec(conn, annotsQuery.c_str());
        if (PQresultStatus(annotsRes) != PGRES_TUPLES_OK) {
            pgError(conn, "SELECT failed: " + String(PQerrorMessage(conn)));
        }
        // first result position & frame number
        int anPos = 0;
        int anFrm = 0; // actual frame number
        if (PQntuples(annotsRes) > 0) anFrm = atoi(PQgetvalue(annotsRes, 0, 0));

        // select states data
        cout << "Loading states ... " << endl;
        string statesQuery = string("SELECT st.time, st.position[0]::integer, st.position[1]::integer, st.size[0]::integer, st.size[1]::integer, st.track, sunar.overlaps(anst.position, anst.size, st.position, st.size)\n") +
            " FROM ONLY sunar.states st LEFT JOIN ONLY sunar.annotation_states anst\n" +
            "   ON st.dataset=anst.dataset AND st.video=anst.video AND st.camera=anst.camera AND (anst.time = st.time OR anst.time = (st.time-1))\n" +
            " WHERE st.dataset="+ adst +" AND st.video="+ avid +" AND st.time > ("+ afst +"-25) AND st.time < ("+ alst +"+25) AND st.camera="+ acam +"\n" +
            " ORDER BY time\n";
        PGresult* statesRes = PQexec(conn, statesQuery.c_str());
        if (PQresultStatus(statesRes) != PGRES_TUPLES_OK) {
            pgError(conn, "SELECT failed: " + String(PQerrorMessage(conn)));
        }
        // first result position & frame number
        int stPos = 0;
        int stFrm = 0; // actual frame number
        if (PQntuples(statesRes) > 0) stFrm = atoi(PQgetvalue(statesRes, 0, 0));

        // DEBUG
        // cout << statesQuery << endl;

        //
        // cykli po vysledcich... 
        // get next video frame & draw the information...
        cout << "Playing!" << endl << endl;
        while (ffPosition(media) <= alast && dc != 'q' && dc != 'w') {
            ffCvFrame(media); // get next video frame
            if (media->ffEof) break;

            // clone the image to be able to draw
            IplImage* image = cvCloneImage(media->sws->cvFrame);

            // print the dataset, camera, video and frame number
            sprintf(buf, "%lu", ffPosition(media));
            string info = string("| Sunar | dataset ")+ adst +" | video "+ avid +" | camera "+ acam +" | frame "+ buf + " |";
            cvPutText(image, info.c_str(), cvPoint(10,20), &boldFont, cvBlack);
            cvPutText(image, info.c_str(), cvPoint(10,20), &font, cvYellow);

            // draw states rectangles (same old or older as the frame)
            int sti = stPos;
            while (sti < PQntuples(statesRes) && stFrm == atoi(PQgetvalue(statesRes, sti, 0))) {
                // zjisti co se ma malovat
                int x = atoi(PQgetvalue(statesRes, sti, 1));
                int y = atoi(PQgetvalue(statesRes, sti, 2));
                int w = atoi(PQgetvalue(statesRes, sti, 3));
                int h = atoi(PQgetvalue(statesRes, sti, 4));
                char* track = PQgetvalue(statesRes, sti, 5);
                char* overlap = PQgetvalue(statesRes, sti, 6);
                
                // maluuuuj
                cvRectangle(image, cvPoint(x-w/2, y-h/2), cvPoint(x+w/2, y+h/2), cvGreen, 1, CV_AA);
                // print the track number & overlapping coef.
                cvPutText(image, track, cvPoint(x+w/2+1, y-h/2+15), &boldFont, cvBlack);
                cvPutText(image, track, cvPoint(x+w/2+1, y-h/2+15), &font, cvGreen);
                if (overlap != null && strcmp(overlap, "0") != 0) {
                    cvPutText(image, overlap, cvPoint(x+w/2+1, y-h/2+30), &boldFont, cvBlack);
                    cvPutText(image, overlap, cvPoint(x+w/2+1, y-h/2+30), &font, cvGreen);
                }
                
                sti++;  // skonci o 1 vyssi, nez je pouzitelne
            }
            // jestli uz nasledujici data ve vysledku dotazu states patri nasledujicimu snimku...
            if (sti < PQntuples(statesRes) && ffPosition(media)+1 == atoi(PQgetvalue(statesRes, sti, 0))) {
                stPos = sti;
                stFrm = atoi(PQgetvalue(statesRes, sti, 0));
            }

            // draw annotation rectangles (same old or older as the frame)
            int ani = anPos;
            while (ani < PQntuples(annotsRes) && anFrm == atoi(PQgetvalue(annotsRes, ani, 0))) {
                // zjisti co se ma malovat
                int x = atoi(PQgetvalue(annotsRes, ani, 1));
                int y = atoi(PQgetvalue(annotsRes, ani, 2));
                int w = atoi(PQgetvalue(annotsRes, ani, 3));
                int h = atoi(PQgetvalue(annotsRes, ani, 4));
                char* ocl = PQgetvalue(annotsRes, ani, 5);

                // maluuuuj
                cvRectangle(image, cvPoint(x-w/2, y-h/2), cvPoint(x+w/2, y+h/2), cvRed, 1, CV_AA);
                // print the track number & object number && f/t occluded
                cvPutText(image, antr, cvPoint(x+w/2, y-h/2+15), &boldFont, cvBlack);
                cvPutText(image, antr, cvPoint(x+w/2, y-h/2+15), &font, cvRed);
                cvPutText(image, aobj, cvPoint(x-w/2+1, y-h/2-10), &boldFont, cvBlack);
                cvPutText(image, aobj, cvPoint(x-w/2+1, y-h/2-10), &font, cvRed);
                cvPutText(image, ocl, cvPoint(x-w/2+1, y-h/2+15), &boldFont, cvBlack);
                cvPutText(image, ocl, cvPoint(x-w/2+1, y-h/2+15), &font, cvWhite);

                ani++;  // skonci o 1 vyssi, nez je pouzitelne
            }
            // jestli uz nasledujici data ve vysledku dotazu states patri nasledujicimu snimku...
            if (ani < PQntuples(annotsRes) && ffPosition(media)+1 == atoi(PQgetvalue(annotsRes, ani, 0))) {
                anPos = ani;
                anFrm = atoi(PQgetvalue(annotsRes, ani, 0));
            }

            // show the image
            cvShowImage("Sunar HMI", image);
            dc = cvWaitKey(dc_delay);  // because of bs
            // set the speed
            dc_delay = setSpeed(dc);

            // release the clone
            cvReleaseImage(&image);
        }  // while position <= alst+25

        if (media != null) ffClose(media); // close the video
        PQclear(annotsRes);
        PQclear(statesRes);
        if (dc == 'w') dc = '3';    // if exited the video - do not exit an other
        if (dc == 'q') break;       // exit (w only plays other video)
    } // 4 annotations

    PQclear(objectsRes);
}

/**
 *  Prints the error to the cerr and exits nicely
 */
void pgError(PGconn *conn, String message) {
    cerr << "Error (PostgreSQL): " << message << endl;

    PQfinish(conn);
    if (media != null) ffClose(media);
    exit(EXIT_FAILURE);
}

/**
 *  Prints the error to the cerr and exits nicely
 */
void error(String message) {
    cerr << message << endl;

    PQfinish(conn);
    if (media != null) ffClose(media);
    exit(EXIT_FAILURE);
}


/**
 * Just for fun ... seting the playback speed
 */
int setSpeed(char dc) {

    switch (dc) {
        case ' ': return dc_delay = 0;
        case '1': return dc_delay = 120;
        case '2': return dc_delay = 78;
        case '3': return dc_delay = 38;
        case '4': return dc_delay = 18;
        case '5': return dc_delay = 8;
        default: return dc_delay;
    }
}












/**
 * TODO: remake!
 */
void browseHandovers(String videoLocation, String datasetStart, String videoStart) {
    cout << "Preparing to start the Sunar HMI handovers presentation... " << endl;
    cout << "To skip the video, press 'w', to quit press 'q'." << endl;
    cout << "The video speed is controled by '1'..'5', 'space' is for steping." << endl << endl;

    // temp. char buffer for itoa, sprintf - do not use 1 buf multiple times per command
    char* buf = alloc(char, 32);
    char dc = '3';
    dc_delay = setSpeed(dc);

    conninfo = CONNINFO;

    // Make a connection to the database
    conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK) {
        pgError(conn, "Connection to database failed: " + String(PQerrorMessage(conn)));
    }

    // Eg. "select * from pg_database"
    String objectsQuery = String("SELECT antr.dataset, antr.object, antr.video, antr.firsts[1], antr.lasts[1], antr.camera, antr.track, vs.name \n") +
            "  FROM ONLY sunar.tracks AS antr JOIN sunar.videos AS vs \n" +
            "    ON antr.dataset=vs.dataset AND antr.camera=vs.camera AND antr.video=vs.video \n" +
            " WHERE object IS NOT NULL AND object < 10000 \n" +
            "   AND antr.dataset="+ datasetStart +" AND antr.video="+ videoStart +" \n" +
            " ORDER BY dataset, object, video, firsts[1], lasts[1], camera, track \n";
        // results
    PGresult* objectsRes = PQexec(conn, objectsQuery.c_str());
    if (PQresultStatus(objectsRes) != PGRES_TUPLES_OK) {
        pgError(conn, "SELECT failed: " + String(PQerrorMessage(conn)));
    }


    // display a display
    cvNamedWindow("Sunar HMI", 0);
    CvFont font, boldFont, largeFont;
    cvInitFont(&font, CV_FONT_HERSHEY_PLAIN, 1.0, 1.2, 0, 1, 8); // or CV_AA instead of 8 and anti-alias...
    cvInitFont(&boldFont, CV_FONT_HERSHEY_PLAIN, 1.0, 1.2, 0, 2, 8);
    cvInitFont(&largeFont, CV_FONT_HERSHEY_PLAIN, 1.8, 2.1, 0, 2, 8);

    // rememver last video name
    string vnameLast = "";

    //
    // go through annotations (videos, tracks and states)
    //
    for (int oi = 0; oi < PQntuples(objectsRes); oi++) {
        char* adst = PQgetvalue(objectsRes, oi, 0);
        char* aobj = PQgetvalue(objectsRes, oi, 1);
        char* avid = PQgetvalue(objectsRes, oi, 2);
        char* afst = PQgetvalue(objectsRes, oi, 3);
        char* alst = PQgetvalue(objectsRes, oi, 4);
        char* acam = PQgetvalue(objectsRes, oi, 5);
        char* antr = PQgetvalue(objectsRes, oi, 6);
        string vname = string(videoLocation) + "/" + PQgetvalue(objectsRes, oi, 7);

        // show the video with the following bounds
        int afirst = (atoi(afst)-25 < 1) ? 1 : atoi(afst)-25; // 25 frames before first frame of the annotation or the first one
        int alast = atoi(alst)+25;  // doesnt matter its more than the video lasts...


        //
        // show the video
        // if the video has changed this time... open a new one
        if (vname.compare(vnameLast) != 0) {
            cout << "Loading a new video ... " << endl;
            vnameLast = vname;

            // allocate new media (or assign null!)
            media = newMedia();
            // open the media file
            if ( !ffOpenFile(media, vname.c_str()) ) {
                error(string("Couldn't open file ") + vname);
            }
        }

        // print seeking...
        cout << "Sunar is seeking ... " << endl;
        // draw the upcomming video information
        IplImage* image = cvCloneImage(media->sws->cvFrame);
        if (image->imageData == null) cvCreateImageData(image);
        string info = string("| Sunar | dataset ")+ adst +" | video "+ avid +" | camera "+ acam +" |";
        cvRectangle(image, cvPoint(0, 0), cvPoint(image->width-1, image->height-1), cvBlack, CV_FILLED, CV_AA);
        cvPutText(image, info.c_str(), cvPoint(10,20), &font, cvYellow);
        cvPutText(image, "Sunar is loading a new video ..." , cvPoint(10,200), &largeFont, cvWhite);
        cvShowImage("Sunar HMI", image);
        cvWaitKey(dc_delay);  // because of bs
        cvReleaseImage(&image);

        // seek to the new video position
        ffCvFramePreciseSeek(media, afirst-1);
        if (media->ffEof) continue;

        //
        // select annotation_states data
        // and init positions and frames
        cout << "Loading handovers ... " << endl;
        string annotsQuery = string("SELECT anst.time, anst.position[0]::integer, anst.position[1]::integer, anst.size[0]::integer, anst.size[1]::integer, anst.occlusion\n") +
            " FROM ONLY sunar.states anst\n" +
            " WHERE anst.dataset="+ adst +" AND anst.video="+ avid +" AND anst.track="+ antr +" AND anst.camera="+ acam +"\n" +
            " ORDER BY time";
        PGresult* annotsRes = PQexec(conn, annotsQuery.c_str());
        if (PQresultStatus(annotsRes) != PGRES_TUPLES_OK) {
            pgError(conn, "SELECT failed: " + String(PQerrorMessage(conn)));
        }
        // first result position & frame number
        int anPos = 0;
        int anFrm = 0; // actual frame number
        if (PQntuples(annotsRes) > 0) anFrm = atoi(PQgetvalue(annotsRes, 0, 0));

        // select states data
        cout << "Loading states ... " << endl;
        string statesQuery = string("SELECT st.time, st.position[0]::integer, st.position[1]::integer, st.size[0]::integer, st.size[1]::integer, st.track\n") +
            " FROM ONLY sunar.states st \n" +
            " WHERE st.track<>"+ antr +" AND st.dataset="+ adst +" AND st.video="+ avid +" AND st.time > ("+ afst +"-25) AND st.time < ("+ alst +"+25) AND st.camera="+ acam +"\n" +
            " ORDER BY time\n";
        PGresult* statesRes = PQexec(conn, statesQuery.c_str());
        if (PQresultStatus(statesRes) != PGRES_TUPLES_OK) {
            pgError(conn, "SELECT failed: " + String(PQerrorMessage(conn)));
        }
        // first result position & frame number
        int stPos = 0;
        int stFrm = 0; // actual frame number
        if (PQntuples(statesRes) > 0) stFrm = atoi(PQgetvalue(statesRes, 0, 0));

        // DEBUG
        // cout << statesQuery << endl;

        //
        // cykli po vysledcich...
        // get next video frame & draw the information...
        cout << "Playing!" << endl << endl;
        while (ffPosition(media) <= alast && dc != 'q' && dc != 'w') {
            ffCvFrame(media); // get next video frame
            if (media->ffEof) break;

            // clone the image to be able to draw
            IplImage* image = cvCloneImage(media->sws->cvFrame);

            // print the dataset, camera, video and frame number
            sprintf(buf, "%lu", ffPosition(media));
            string info = string("| Sunar | dataset ")+ adst +" | video "+ avid +" | camera "+ acam +" | frame "+ buf + " |";
            cvPutText(image, info.c_str(), cvPoint(10,20), &boldFont, cvBlack);
            cvPutText(image, info.c_str(), cvPoint(10,20), &font, cvYellow);

            // draw states rectangles (same old or older as the frame)
            int sti = stPos;
            while (sti < PQntuples(statesRes) && stFrm == atoi(PQgetvalue(statesRes, sti, 0))) {
                // zjisti co se ma malovat
                int x = atoi(PQgetvalue(statesRes, sti, 1));
                int y = atoi(PQgetvalue(statesRes, sti, 2));
                int w = atoi(PQgetvalue(statesRes, sti, 3));
                int h = atoi(PQgetvalue(statesRes, sti, 4));
                char* track = PQgetvalue(statesRes, sti, 5);

                // maluuuuj
                cvRectangle(image, cvPoint(x-w/2, y-h/2), cvPoint(x+w/2, y+h/2), cvGreen, 1, CV_AA);
                // print the track number & overlapping coef.
                cvPutText(image, track, cvPoint(x+w/2+1, y-h/2+15), &boldFont, cvBlack);
                cvPutText(image, track, cvPoint(x+w/2+1, y-h/2+15), &font, cvGreen);

                sti++;  // skonci o 1 vyssi, nez je pouzitelne
            }
            // jestli uz nasledujici data ve vysledku dotazu states patri nasledujicimu snimku...
            if (sti < PQntuples(statesRes) && ffPosition(media)+1 == atoi(PQgetvalue(statesRes, sti, 0))) {
                stPos = sti;
                stFrm = atoi(PQgetvalue(statesRes, sti, 0));
            }

            // draw annotation rectangles (same old or older as the frame)
            int ani = anPos;
            while (ani < PQntuples(annotsRes) && anFrm == atoi(PQgetvalue(annotsRes, ani, 0))) {
                // zjisti co se ma malovat
                int x = atoi(PQgetvalue(annotsRes, ani, 1));
                int y = atoi(PQgetvalue(annotsRes, ani, 2));
                int w = atoi(PQgetvalue(annotsRes, ani, 3));
                int h = atoi(PQgetvalue(annotsRes, ani, 4));
                char* ocl = PQgetvalue(annotsRes, ani, 5);

                // maluuuuj
                cvRectangle(image, cvPoint(x-w/2, y-h/2), cvPoint(x+w/2, y+h/2), cvRed, 1, CV_AA);
                // print the track number & object number && f/t occluded
                cvPutText(image, antr, cvPoint(x+w/2, y-h/2+15), &boldFont, cvBlack);
                cvPutText(image, antr, cvPoint(x+w/2, y-h/2+15), &font, cvRed);
                cvPutText(image, aobj, cvPoint(x-w/2+1, y-h/2-10), &boldFont, cvBlack);
                cvPutText(image, aobj, cvPoint(x-w/2+1, y-h/2-10), &font, cvRed);
                cvPutText(image, ocl, cvPoint(x-w/2+1, y-h/2+15), &boldFont, cvBlack);
                cvPutText(image, ocl, cvPoint(x-w/2+1, y-h/2+15), &font, cvWhite);

                ani++;  // skonci o 1 vyssi, nez je pouzitelne
            }
            // jestli uz nasledujici data ve vysledku dotazu states patri nasledujicimu snimku...
            if (ani < PQntuples(annotsRes) && ffPosition(media)+1 == atoi(PQgetvalue(annotsRes, ani, 0))) {
                anPos = ani;
                anFrm = atoi(PQgetvalue(annotsRes, ani, 0));
            }

            // show the image
            cvShowImage("Sunar HMI", image);
            dc = cvWaitKey(dc_delay);  // because of bs
            // set the speed
            dc_delay = setSpeed(dc);

            // release the clone
            cvReleaseImage(&image);
        }  // while position <= alst+25

        if (media != null) ffClose(media); // close the video
        PQclear(annotsRes);
        PQclear(statesRes);
        if (dc == 'w') dc = '3';    // if exited the video - do not exit an other
        if (dc == 'q') break;       // exit (w only plays other video)
    } // 4 annotations

    PQclear(objectsRes);
}
