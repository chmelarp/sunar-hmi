/**
 * File:   test.cc
 * Author: chmelarp
 *
 * Created on 11. červen 2008, 9:59
 * 
 * Test the C version of libpq, the PostgreSQL frontend library in C++.
 */

// Include C/C++
#include <stdlib.h>
#include <iostream>
#include <string>
// PostgreSQL
#include <postgresql/libpq-fe.h>
// #include <libpq-fe.h>    // sometimes... 
// custom
#include "cvffmpeg/abbrevs.h"


/**
 *  Prints the error to the cerr and exits nicely
 */
void pgError(PGconn *conn, String message) {
    cerr << "Error (PostgreSQL): " << message << endl;

    PQfinish(conn);
    exit(1);
}

/**
 * Main fun
 * Arguments: host=minerva2 port=5432 dbname=trecvid user=trecvid password='*******' sslmode=require
 */
int main(int argc, char **argv) {
    const char *conninfo;   // Connection info string
    PGconn     *conn;       // Connection
    PGresult   *res;        // The result (set)

    /** ************************************************************************************************
     * If the user supplies a parameter on the command line, use it as the
     * conninfo string; otherwise default to setting dbname=TRECVid...
     */
    
    if (argc > 1)
        conninfo = argv[1]; // TODO: this won't work... (nebrat)
    else
        conninfo = "host=minerva2 port=5432 dbname=trecvid user=trecvid password='f2wipufi' sslmode=require";

    // Make a connection to the database 
    conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK) {
        pgError(conn, "Connection to database failed: " + String(PQerrorMessage(conn)));
    }
    
    /** ************************************************************************************************
     * Delete some data in the table 'tv2_test'
     * PQexec() of a query command that doesn't return annything and was 
     * executed properly by the backend returns PGRES_COMMAND_OK .
     */
    res = PQexec(conn, "delete from tv2_test where video=1 and frame=50 and word=55 and id=1");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        pgError(conn, "SELECT failed: " + String(PQerrorMessage(conn)));
    }
    PQclear(res);

    
    /** ************************************************************************************************
     * Insert some data in the table 'tv2_test'
     */
    String cmd = "insert into tv2_test(video, frame, word, id) values(1, 50, 55, 1)"; // x, y, scale is null
    
    if (PQresultStatus(res = PQexec(conn, cmd.c_str())) != PGRES_COMMAND_OK) {
        pgError(conn, "SELECT failed: " + String(PQerrorMessage(conn)));
    }
    PQclear(res);

    /** ************************************************************************************************
     * Select (and fetch) some data (rows, tuples) from the database.
     * Eg. "select * from pg_database" (the system catalog of databases)
     */
    res = PQexec(conn, "select * from tv2_test");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        pgError(conn, "SELECT failed: " + String(PQerrorMessage(conn)));
    }
    
    // first, print out the attribute names (header)
    int nFields = PQnfields(res);
    for (int i = 0; i < nFields; i++) {
        printf("%-15s", PQfname(res, i));
    }
    printf("\n\n");
    
    // next, print out the rows (data) 
    for (int i = 0; i < PQntuples(res); i++) {
        for (int j = 0; j < nFields; j++) {
            printf("%-15s", PQgetvalue(res, i, j));
        }
        printf("\n");
    }
   
    PQclear(res);
    
    /** ************************************************************************************************ 
     * Close the connection to the database and cleanup.
     */
    PQfinish(conn);
    
    return (EXIT_SUCCESS);
}
