/*
 * File:   browseAnnots.cpp
 *
 * Author: Petr Chmelar (c) 2010
 *         Jozef Mlich (thanks for 5 camera merge)
 * License: GNU/GPL and Intel CV
 *
 * Created on 01. 08. 2010, 00:01
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

// Postgres
const char *conninfo = null;   // Connection info string
PGconn     *conn = null;       // Connection

// cv streams and images
CvCapture** videos = null;
CvVideoWriter* video_out = null;
IplImage** images = null;
IplImage* image_out = null;

int frame = 0; // real... use (FrameNum % VPSTEP == 1) ... process not-null frames 1, (3,) 5, (7,) 9, ...

// Just for fun - delay for video speed control
char dc_delay = 38;


/**
 * 
 * @param videoLocation
 * @param trackStr
 */
int browseExperiments(String videoLocation, String trackStr, String videoPrefix) {
    // set locale to original POSIX / ANSI C
    setlocale (LC_NUMERIC, "us");

    cout << "SUNAR HMI is preparing to start the presentation (track " << trackStr << ")... " << endl;
    boolean first = true;

    // TODO: implement speed changes
    char dc = '3';
    dc_delay = setSpeed(dc);

    // Make a connection to the database
    conninfo = CONNINFO;
    conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK) {
        cerr << "Error! Connection to database failed: " << String(PQerrorMessage(conn)) << endl;
        return 1;
    }

    // find the experiment states --------------------------------------------------------
    int dataset = 0;
    int video = 0;
    int start_time = 0;
    int end_time = 0;
    int exps_cnt;
    stringstream exps_query;
    exps_query << "SELECT e.camera, e.\"time\", e.position[0], e.position[1], e.size[0], e.size[1], e.track, e.prob*1000, e.dataset, e.video FROM ONLY sunar.processed e\n" <<
            " WHERE e.experiment= " << trackStr << "\n" << // " AND dataset<=4 \n" <<
            " ORDER BY e.\"time\", e.dataset, e.video, e.camera \n";

    PGresult* exps_res = PQexec(conn, exps_query.str().c_str());
    if (PQresultStatus(exps_res) != PGRES_TUPLES_OK) {
        cout << exps_query.str();
        pgError(conn, "SELECT failed: " + String(PQerrorMessage(conn)));
        return 1;
    }
    else {
        exps_cnt = PQntuples(exps_res);
        start_time = atoi(PQgetvalue(exps_res, 0, 1));
        start_time = (start_time-50 < 1) ? 1 : start_time - 50; // start 2 seconds in advance
        end_time = atoi(PQgetvalue(exps_res, exps_cnt-1, 1)) + 50; // end 2 seconds later
        dataset = atoi(PQgetvalue(exps_res, 0, 8));
        video =     atoi(PQgetvalue(exps_res, 0, 9));
        cout << "The track starts at frame " << start_time << ", dataset " << dataset << ", video " << video << " (" << exps_cnt << " states)" << endl;
    } // jestli tohle zhuci, tak smula


    // find the overlapping annotation (begin, end) ----------------------------
    int annots_cnt = 0;
    stringstream annots_query;
    annots_query << "SELECT a.camera, a.\"time\", a.position[0], a.position[1], a.size[0], a.size[1] FROM ONLY sunar.annotations a \n" <<
        " WHERE a.dataset=" << dataset << " AND a.video=" << video << " AND a.\"time\">" << start_time <<"-1000 AND a.\"time\"<" << end_time <<"+1000 \n" <<
        " ORDER BY a.\"time\", a.camera \n";
    PGresult* annots_res = PQexec(conn, annots_query.str().c_str());
    if (PQresultStatus(annots_res) != PGRES_TUPLES_OK) {
        cout << annots_query.str();
        cerr << "No annotations found: " + String(PQerrorMessage(conn)) << endl;
    }
    else {
        annots_cnt = PQntuples(annots_res);
        int first = atoi(PQgetvalue(annots_res, 0, 1));
        if (first-25 < start_time) start_time = (first-25 < 1) ? 1 : first-25;
        int last = atoi(PQgetvalue(annots_res, annots_cnt-1, 1));
        if (last+25 > end_time) end_time = last+25;
        cout << "The annotation starts at frame " << start_time << " (" << annots_cnt << " states)" << endl;
    } // jestli tohle zhuci, tak neva


    // find annotation trajectories and other states
    int states_cnt = 0;
    stringstream states_query;
    cout << "Loading annotations... ";
    states_query << "SELECT st.camera, st.time, st.position[0], st.position[1], st.size[0], st.size[1], st.track, "
            " (sunar.overlaps(anst.position, anst.size, st.position, st.size)*1000)::integer, st.object, st.experiment\n" <<
            " FROM ONLY sunar.processed st LEFT JOIN ONLY sunar.annotations anst\n" <<
            "   ON st.dataset=anst.dataset AND st.video=anst.video AND st.camera=anst.camera AND st.object=anst.object AND (st.time >= anst.time-2) AND (st.time <= anst.time+2)\n" <<
            " WHERE st.dataset=" << dataset << " AND st.video=" << video << " AND st.time > (" << start_time << "-4) AND st.time < (" << end_time << "+4) \n" <<
            " ORDER BY st.time, st.camera \n";
    // cout << states_query.str() << endl;
    PGresult* states_res = PQexec(conn, states_query.str().c_str());
    if (PQresultStatus(states_res) != PGRES_TUPLES_OK) {
        cout << states_query.str() << endl;
        cerr << "Error! SELECT failed: " + String(PQerrorMessage(conn)) << endl;
    }
    else {
        states_cnt = PQntuples(states_res);
        cout << states_cnt << " states loaded." << endl;
    } // jestli tohle zhuci, tak neva



    // find videos -------------------------------------------------------------
    int videos_cnt = 0;
    stringstream videos_query;
    videos_query << "SELECT v.camera, v.\"name\", v.length FROM ONLY sunar.videos v \n" <<
        " WHERE v.dataset=" << dataset << " AND v.video=" << video << " \n" <<
        " ORDER BY v.camera \n";

    PGresult* videos_res = PQexec(conn, videos_query.str().c_str());
    if (PQresultStatus(videos_res) != PGRES_TUPLES_OK) {
        cout << videos_query.str() << endl;
        cerr << "Error! SELECT failed: " + String(PQerrorMessage(conn)) << endl;
        return 1;
    }
    else {
        videos_cnt = PQntuples(videos_res);
        // TODO: Jozin: dal je to natvrdo na 5
        if (videos_cnt != 5 ) {
            cerr << "Unimplemented: videos_cnt = " << videos_cnt << " currently supported 5 only"<< endl;
            return 1;
        }

        // otevri ty videja
        cout << "Opening " << videos_cnt << " videos:" << endl;
        videos = (CvCapture**) malloc( sizeof (CvCapture*) * videos_cnt);
        for (int vi = 0; vi < videos_cnt; ++vi) {
            char* vname = PQgetvalue(videos_res, vi, 1);
            char buf[512];
            snprintf(buf, 512, "%s/%s", videoLocation.c_str(), vname);
            cout << buf << endl;
            videos[vi] = cvCaptureFromAVI(buf);

            // predpokladam shodnou delku
            int length = atoi(PQgetvalue(videos_res, vi, 2));
            if (length > start_time && end_time > length) end_time = length;
            // TODO: jeste by nemusely byt dobre usporadane
        }

        // nutne stvorit a inicializovat obrazky
        images = (IplImage**) malloc( sizeof (IplImage*) * videos_cnt);
        for (int vi = 0; vi < videos_cnt; ++vi) images[vi] = null;
    } // jestli tohle zhuci, tak jsem v haji zelenem

    // create output video if specified
    if (videoPrefix.compare("") != 0) {
        char buf[512];
        snprintf(buf, 512, "%s%s.avi", videoPrefix.c_str(), trackStr.c_str());
        video_out = cvCreateVideoWriter(buf, CV_FOURCC('X', 'V', 'I', 'D'), 25.0/VPSTEP, cvSize(1080, 576), 1); // 2160x1152, 1920x1024, 1440x768, 1080x576
    }

    cout << endl << "SUNAR HMI o_o seeking o_o" << endl;

    // ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // main loop driven by frame number //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int exps_act = 0;
    int annots_act = 0;
    int states_act = 0;
    for (frame = 1; frame <= end_time; ++frame) {
        
        // get frames
        for (int vi = 0; vi < videos_cnt; ++vi) {
            if (images[vi] == null) images[vi] = cvCreateImage(cvSize(720, 576), 8, 3); // first time

            if (!videos[vi]) {
                cerr << "Error: !videos[" << vi << "]" << endl;
            }
            else if (!cvGrabFrame(videos[vi])) {
                std::cerr << "Errorr: !cvGrabFrame(videos[" << vi << "])" << endl;
            }
            else {
                IplImage* tmpimg = cvRetrieveFrame(videos[vi]);
                if (tmpimg == null) { // tohle udela jen upozorneni... potreba zustane cerny
                    cerr << "Errorr: !cvRetrieveFrame(videos[" << vi << "])" << endl;
                }
                else {
                    cvCopy(tmpimg, images[vi]);
                }
            }
        } // have frames

        // seek
        // cout << frame << endl;
        if (frame < start_time) {
            if (frame % 100 == 0) cout << frame << " ";
            continue; // seek
        }
        if (frame % VPSTEP != 1) continue; // process not-null frames 1, (3,) 5, (7,) 9, ...

        // check video count, null, etc.
        if (images[0] == null || images[1] == null || images[2] == null || images[3] == null || images[4] == null) break; // this is the end
        if (image_out == null) {
            image_out = cvCreateImage(cvSize(2160, 1152), 8, 3); // 3*720 x 2*576
        }
        if (first) {
            cout << endl << "SUNAR HMI >> playing >>" << endl << endl;
            first = false;
        }

        // print frame number --------------------------------------------------
        char str[1024];
        int line_type = CV_AA; // Change it to 8 to see non-antialiased graphics.
        CvFont font;
        cvInitFont( &font, CV_FONT_HERSHEY_PLAIN, 1.5, 1.5, 0, 1, line_type );
        // erase the previously written data (magic constants)
        cvRectangle(image_out, cvPoint(1440, 576), cvPoint(2159, 1151), cvBlack, CV_FILLED);
        // print information
        sprintf(str, "SUNAR HMI");
        cvPutText(image_out, str, cvPoint(1470, 606), &font, cvWhite);
        sprintf(str, "  dataset: %02d video: %02d", dataset, video);
        cvPutText(image_out, str, cvPoint(1470, 634), &font, cvWhite);
        sprintf(str, "  frame: %04d (of %04d-%04d)", frame, start_time, end_time);
        cvPutText(image_out, str, cvPoint(1470, 662), &font, cvWhite);

        // next, the font is smaller
        cvInitFont(&font, CV_FONT_HERSHEY_PLAIN, 1.0, 1.0, 0, 1, line_type);

        // takhle se vybarvi ten annots (referencni) frame
        CvScalar refColor[5];
        for (int i = 0; i < 5; i++) {
            refColor[i] = cvRed;
        }

        // draw states ---------------------------------------------------------
        int states_tmp = states_act;
        while (states_tmp < states_cnt && atoi(PQgetvalue(states_res, states_tmp, 1)) < frame) states_tmp++; // seek first
        states_act = states_tmp; // next time start using the actual frame
        while (states_tmp < states_cnt && atoi(PQgetvalue(states_res, states_tmp, 1)) <= frame+1) { // draw the actual (and possibly 1 future) state
            int c = atoi(PQgetvalue(states_res, states_tmp, 0));        // camera
            int x = atoi(PQgetvalue(states_res, states_tmp, 2));        // x
            int y = atoi(PQgetvalue(states_res, states_tmp, 3));        // y
            int w = atoi(PQgetvalue(states_res, states_tmp, 4));        // width
            int h = atoi(PQgetvalue(states_res, states_tmp, 5));        // height
            int t = atoi(PQgetvalue(states_res, states_tmp, 6));        // track
            float o = atoi(PQgetvalue(states_res, states_tmp, 7))/1000.0; // overlaps ... atof etc. doesn't work :(
            int oo = atoi(PQgetvalue(states_res, states_tmp, 8));       // object
            int ex = atoi(PQgetvalue(states_res, states_tmp, 9));       // experiment

            // draw the blob
            int thickness = 1;
            CvScalar color = cvWhite;
            if (o > 0.2) { 
                thickness = 2;
            }  // vytucni overlapping
            if (oo > 0) {
                color = cvPink;    // probarvi assigned - melo se priradit
            }

            if (ex > 0) {
                color = cvYellow;    // probarvi assigned

                // nasli :)
                if (o > 0.15) {
                    refColor[c-1] = cvGreen;
                }
                else if (refColor[c-1].val[2] > 0) { // if it is red or orange (not green) ... stored as BGRA
                    refColor[c-1] = cvOrange;
                }
            }

            cvRectangle(images[c-1], cvPoint(x - w/2, y - h/2), cvPoint(x + w/2, y + h/2), color, thickness, CV_AA);
            sprintf(str, "%d", t); // print track number
            cvPutText(images[c-1], str, cvPoint(x - w/2, y - h/2), &font, color);

            states_tmp++;
        }
        
        // draw experiment states ---------------------------------------------------------
        int exp_tmp = exps_act;
        while (exp_tmp < exps_cnt && atoi(PQgetvalue(exps_res, exp_tmp, 1)) < frame) exp_tmp++; // seek first
        exps_act = exp_tmp; // next time start using the actual frame
        while (exp_tmp < exps_cnt && atoi(PQgetvalue(exps_res, exp_tmp, 1)) <= frame+1) { // draw the actual (and possibly 1 future) state
            int c = atoi(PQgetvalue(exps_res, exp_tmp, 0));        // camera
            int x = atoi(PQgetvalue(exps_res, exp_tmp, 2));        // x
            int y = atoi(PQgetvalue(exps_res, exp_tmp, 3));        // y
            int w = atoi(PQgetvalue(exps_res, exp_tmp, 4));        // width
            int h = atoi(PQgetvalue(exps_res, exp_tmp, 5));        // height
            int t = atoi(PQgetvalue(exps_res, exp_tmp, 6));        // track
            int p = atoi(PQgetvalue(exps_res, exp_tmp, 6));        // prob*1000

            // draw the blob
            int thickness = 1;
            CvScalar color = cvYellow;
            // this is already done above
            // cvRectangle(images[c-1], cvPoint(x - w/2, y - h/2), cvPoint(x + w/2, y + h/2), color, thickness, CV_AA);
            // sprintf(str, "%d", t); // print track number
            // cvPutText(images[c-1], str, cvPoint(x - w/2, y - h/2), &font, color);

            sprintf(str, "%.3f", (float)p/1000); // print prob
            cvPutText(images[c-1], str, cvPoint(x - w/4, y), &font, color);

            exp_tmp++;
        }


        // draw annots (predbarveny) -------------------------------------------
        int annots_tmp = annots_act;
        while (annots_tmp < annots_cnt && atoi(PQgetvalue(annots_res, annots_tmp, 1)) < frame-2) annots_tmp++; // seek first
        annots_act = annots_tmp; // next time start using the actual frame
        while (annots_tmp < annots_cnt && atoi(PQgetvalue(annots_res, annots_tmp, 1)) <= frame+2) { // draw the actual (and possibly 1 future) state
            int c = atoi(PQgetvalue(annots_res, annots_tmp, 0));        // camera
            int x = atoi(PQgetvalue(annots_res, annots_tmp, 2));        // x
            int y = atoi(PQgetvalue(annots_res, annots_tmp, 3));        // y
            int w = atoi(PQgetvalue(annots_res, annots_tmp, 4));        // width
            int h = atoi(PQgetvalue(annots_res, annots_tmp, 5));        // height

            // draw the blob
            cvRectangle(images[c-1], cvPoint(x - w/2, y - h/2), cvPoint(x + w/2, y + h/2), refColor[c-1], 2, CV_AA);

            annots_tmp++;
        }

        
        // paste videos together (jozin)
        cvSetImageROI( image_out, cvRect( 0, 0, 720, 576 ) );
        cvCopy( images[0], image_out );
        cvSetImageROI( image_out, cvRect( 720, 0, 720, 576 ) );
        cvCopy( images[1], image_out );
        cvSetImageROI( image_out, cvRect( 1440, 0, 720, 576 ) );
        cvCopy( images[2], image_out );
        cvSetImageROI( image_out, cvRect( 0, 576, 720, 576 ) );
        cvCopy( images[3], image_out );
        cvSetImageROI( image_out, cvRect( 720, 576, 720, 576 ) );
        cvCopy( images[4], image_out );
        cvResetImageROI(image_out);

        cvNamedWindow("SUNAR HMI", 0);
        cvShowImage("SUNAR HMI", image_out);
        /*key  = */ cvWaitKey((1000.0/25.0*VPSTEP) -50); // show some real speed

        if (video_out != null && image_out != null) {
            cvWriteFrame(video_out, image_out);
        }
    }

    cout << endl << "SUNAR HMI || finished ||" << endl;

    // cleanup
    for (int vi = 0; vi < videos_cnt; ++vi) {
      if (videos[vi] != null) cvReleaseCapture( &videos[vi] );
      if (images[vi] != null) cvReleaseImage( &images[vi] );
    }
    // if (videos != null) free(videos);
    // if (videos != null) free(videos);

    if (image_out != null) cvReleaseImage(&image_out);
    if (video_out != null) cvReleaseVideoWriter (&video_out);

    cvDestroyAllWindows();
  
    return 0;
}
