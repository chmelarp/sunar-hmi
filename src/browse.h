/*
 * File:   browse.h
 *
 * Author: Petr Chmelar (c) 2009
 * License: GNU/GPL and Intel CV
 *
 * Created on 05. 06. 2009, 17:23
 * Changed on
 */

#ifndef __BROWSE_H__
#define	__BROWSE_H__

#include <string>

// PostgreSQL
#include <postgresql/libpq-fe.h>
// custom
#include "cvffmpeg/abbrevs.h"
#include "cvffmpeg/cvffmpeg.h"

// minerva2
#define CONNINFO "host=localhost port=5432 dbname=trecvid user=trecvid password='f2wipufi'" // sslmode=require

using namespace std;


// initialized elsewhere
// Postgres
extern const char *conninfo;   // Connection info string
extern PGconn     *conn;       // Connection

// Assign null to the new media
extern ffMedia* media;

// Just for fun - delay for video speed control
extern char dc_delay;

// initialized in main.php
extern String actionStr;
       // minerva1 /mnt/minerva1/chmelarp/i-Lids/mct_tr
extern String videoLocation;
extern String datasetStart;
extern String videoStart;
extern String trackStart;


/**
 *
 */
void browseAnnots(String videoLocation, String datasetStart, String videoStart);

/**
 *
 */
void browseHandovers(String videoLocation, String datasetStart, String videoStart);

/**
 * 
 * @param videoLocation
 * @param trackStart
 */
int browseExperiments(String videoLocation, String trackStart, String videoPrefix);

/**
 *  Prints the error to the cerr and exits nicely
 */
void pgError(PGconn *conn, String message);
void error(String message);

/**
 * Just for fun ... seting the playback speed
 */
int setSpeed(char dc);

#endif	/* __BROWSE_H__ */

