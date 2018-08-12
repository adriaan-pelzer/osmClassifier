#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <signal.h>
#include <ngramsLocations.h>
#include <classifier.h>
#include <parseDir.h>
#include <corpusToNgrams.h>

#define DAEMON_NAME "osmClassifier"
#define PID_FILE "/var/run/osmClassifier.pid"
#define NGRAMS_SIZE 7

#define P_EMG LOG_DAEMON|LOG_EMERG
#define P_ALT LOG_DAEMON|LOG_ALERT
#define P_CRT LOG_DAEMON|LOG_CRIT
#define P_ERR LOG_DAEMON|LOG_ERR
#define P_WRN LOG_DAEMON|LOG_WARNING
#define P_NTC LOG_DAEMON|LOG_NOTICE
#define P_INF LOG_DAEMON|LOG_INFO
#define P_DBG LOG_DAEMON|LOG_DEBUG

int sig_restart = 0;

void PrintUsage ( int argc, char *argv[] ) {
    if ( argc > 0 ) {
        printf ( "Usage: %s -n -d folder -c classification config -h\n", argv[0] );
        printf ( "  Options:\n" );
        printf ( "      -n\tDon't daemonize.\n" );
        printf ( "      -d folder\tThe folder to read unclassified texts from.\n" );
        printf ( "      -c classification config\tThe file that contains the classification config.\n" );
        printf ( "      -h\tShow this help screen.\n" );
        printf ( "\n" );
    }
}

void signal_handler ( int sig ) {
    switch (sig) {
        case SIGHUP:
            syslog ( LOG_WARNING, "Received SIGHUP signal." );
            sig_restart = 1;
            break;
        case SIGINT:
            syslog ( LOG_WARNING, "Received SIGINT signal." );
            exit ( EXIT_SUCCESS );
            break;
        case SIGABRT:
            syslog ( LOG_WARNING, "Received SIGABRT signal." );
            exit ( EXIT_SUCCESS );
            break;
        case SIGTERM:
            syslog ( LOG_WARNING, "Received SIGTERM signal." );
            exit ( EXIT_SUCCESS );
            break;
        default:
            syslog ( LOG_WARNING, "Unhandled signal (%d) %s", sig, strsignal ( sig ) );
            break;
    }
}

int process_file ( fileCtx_t *fileCtx, ngramsLocations_t *ngramsLocations ) {
    int rc = EXIT_FAILURE;
    char *content = NULL;
    ngrams_t *ngrams = NULL;
    size_t i = 0, j = 0;

    if ( ( content = get_file_content ( fileCtx ) ) == NULL ) {
        syslog ( P_ERR, "Cannot get file content: %m" );
        if ( close_unlock_file ( fileCtx ) != EXIT_SUCCESS )
            syslog ( P_ERR, "Cannot close and unlock file: %m" );
        goto over;
    }

    if ( ( ngrams = ngrams_init ( NGRAMS_SIZE ) ) == NULL ) {
        syslog ( P_ERR, "Cannot initialise ngrams: %m" );
        goto over;
    }

    if ( ngrams_corpus_parse ( content, strlen ( content ), ngrams ) != EXIT_SUCCESS ) {
        syslog ( P_ERR, "Cannot parse corpus: %m" );
        goto over;
    }

    for ( i = 0; i < ngrams->max_order; i++ ) {
        for ( j = 0; j < ngrams->orderset[i].size; j++ ) {
            LatLongList_t *lll = NULL;
            size_t k = 0;

            if ( ( lll = osmClassifier_lookup ( ngramsLocations, ngrams->orderset[i].ngram[j] ) ) == NULL ) {
                if ( errno != 0 ) {
                    syslog ( P_ERR, "Cannot perform lookup of phrase '%s': %m", ngrams->orderset[i].ngram[j] );
                    goto over;
                }
            }

            printf ( "%s:", ngrams->orderset[i].ngram[j] );

            if ( lll )
                for ( k = 0; k < lll->size; k++ )
                    printf ( " | %lf,%lf", (double) lll->lat_lon[k].lat, (double) lll->lat_lon[k].lon );
            else
                printf ( " not found" );

            printf ( "\n" );
        }
    }

    if ( close_unlink_file ( fileCtx ) != EXIT_SUCCESS ) {
        syslog ( P_ERR, "Cannot close and unlink file: %m" );
        goto over;
    }

    rc = EXIT_SUCCESS;
over:
    if ( content )
        free ( content );

    if ( ngrams )
        ngrams_free ( ngrams );

    return rc;
}

int main ( int argc, char **argv ) {
    int c;
#ifdef DEBUG
    int daemonize = 0;
#else
    int daemonize = 1;
#endif
    char *inputFldr = NULL;
    char *ngramFile = NULL;
    pid_t pid, sid;
    ngramsLocations_t *ngramsLocations = NULL;

    while ( ( c = getopt ( argc, argv, "nd:c:h|help" ) ) != -1 ) {
        switch ( c ) {
            case 'h':
                PrintUsage ( argc, argv );
                exit ( EXIT_SUCCESS );
                break;
            case 'n':
                daemonize = 0;
                break;
            case 'd':
                inputFldr = optarg; 
                break;
            case 'c':
                ngramFile = optarg; 
                break;
            default:
                PrintUsage ( argc, argv );
                exit ( EXIT_SUCCESS );
                break;
        }
    }

    if ( inputFldr == NULL ) {
        fprintf ( stderr, "Specify a folder to get texts from\n\n" );
        PrintUsage ( argc, argv );
        exit ( EXIT_FAILURE);
    }

    if ( ngramFile == NULL ) {
        fprintf ( stderr, "Specify a file that contains the classification config\n\n" );
        PrintUsage ( argc, argv );
        exit ( EXIT_FAILURE);
    }

    signal ( SIGHUP, signal_handler );
    signal ( SIGTERM, signal_handler );
    signal ( SIGINT, signal_handler );
    signal ( SIGQUIT, signal_handler );

#ifdef DEBUG
    setlogmask ( LOG_UPTO ( LOG_DEBUG ) );
    openlog ( DAEMON_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER );
#else
    setlogmask ( LOG_UPTO ( LOG_INFO ) );
    openlog ( DAEMON_NAME, LOG_CONS, LOG_USER );
#endif

    if ( daemonize ) {
        syslog ( P_INF, "%s daemon starting up", DAEMON_NAME );

        if ( ( pid = fork () ) < 0 ) {
            syslog ( P_ERR, "Could not fork child process: %m" );
            exit ( EXIT_FAILURE );
        } else if ( pid > 0 ) {
            syslog ( P_NTC, "Child process successfully forked" );
            exit ( EXIT_SUCCESS );
        }

        umask ( 0 );
        sid = setsid ();

        if ( sid < 0 ) {
            syslog ( P_ERR, "Could not create new session: %m" );
            exit ( EXIT_FAILURE );
        }

        if ( ( chdir ( "/" ) ) < 0 ) {
            syslog ( P_ERR, "Could not change working dir: %m" );
            exit ( EXIT_FAILURE );
        }

        close ( STDIN_FILENO );
        close ( STDOUT_FILENO );
        close ( STDERR_FILENO );
    }

    if ( ( ngramsLocations = osmClassifier_init ( ngramFile ) ) == NULL ) {
        syslog ( P_ERR, "Cannot initialise classifier from config: %m" );
        exit ( EXIT_FAILURE );
    }


    for ( ; ; ) {
        fileCtx_t *fileCtx = open_lock_oldest_unlocked_file ( inputFldr );

        if ( fileCtx ) {
            if ( process_file ( fileCtx, ngramsLocations ) != EXIT_SUCCESS )
                exit ( EXIT_FAILURE );
        } else {
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 100000000;
#ifdef DEBUG
            syslog ( P_INF, "No file at this time: %s", strerror ( errno ) );
            exit ( EXIT_SUCCESS );
#endif
            nanosleep ( &ts, NULL );
        }
    }

    syslog ( P_INF, "%s shutting down", DAEMON_NAME );
    exit ( EXIT_SUCCESS );
}
