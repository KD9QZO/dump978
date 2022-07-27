//
// Copyright 2015, Oliver Jowett <oliver@mutability.co.uk>
//
//
// This file is free software: you may copy, redistribute and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation, either version 2 of the License, or (at your
// option) any later version.
//
// This file is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <time.h>
#include <sys/select.h>
#include <errno.h>

#include "uat.h"
#include "uat_decode.h"
#include "reader.h"



#define NON_ICAO_ADDRESS (0x1000000U)


/*! \brief Data structure describing an aircraft */
struct aircraft {
	struct aircraft *next;
	uint32_t address;					/*!< The \e address of the aircraft */

	uint32_t messages;					/*!< The number of \e messages that have been received from the aircraft */
	time_t last_seen;					/*!< The \ref time_t value at which the aircraft was last "seen" */
	time_t last_seen_pos;				/*!< The \ref time_t value at which the aircraft was last "seen" <em>with position data</em> */

	/* data validity flags */
	int position_valid:1;				/*!< A \b boolean flag indicating whether the <em>position data</em> is valid */
	int altitude_valid:1;				/*!< A \b boolean flag indicating whether the <em>altitude data</em> is valid */
	int track_valid:1;					/*!< A \b boolean flag indicating whether the <em>track data</em> is valid */
	int speed_valid:1;					/*!< A \b boolean flag indicating whether the <em>speed data</em> is valid */
	int vert_rate_valid:1;				/*!< A \b boolean flag indicating whether the <em>vertical rate data</em> is valid */

	airground_state_t airground_state;	/*!< Whether the aircraft is <em>on the ground</em>, or <em>in the air</em> */
	char callsign[9];					/*!< The \e callsign of the aircraft */
	char squawk[9];						/*!< The transponder \e squawk code of the aircraft */

	/* if position_valid: */
	double lat;							/*!< The \e latitude (location) of the aircraft */
	double lon;							/*!< The \e longitude (location) of the aircraft */

	/* if altitude_valid: */
	int32_t altitude;					/*!< The \e altitude of the aircraft, in \b ft (feet) */

	/* if track_valid: */
	uint16_t track;						/*!< The \e track the aircraft is following, in \b degrees */

	/* if speed_valid: */
	uint16_t speed;						/*!< The \e speed of the aircraft, in \b kts (knots) */

	/* if vert_rate_valid: */
	int16_t vert_rate;					/*!< The \e ascent or \e descent rate of the aircraft, in \b fpm (feet per minute) */
};

/*! \brief Holds the information about the JSON file directory and file names */
struct json_file_info {
	const char *json_data_path;			/*!< The path within the filesystem that the JSON files are to be written */
	const char *receiver_info_name;		/*!< The filename of the receiver info JSON file */
	const char *receiver_info_name_new;	/*!< The filename of a newly-created receiver info JSON file */
	const char *aircraft_info_name;		/*!< The filename of the aircraft data JSON file */
	const char *aircraft_info_name_new;	/*!< The filename of a newly-created aircraft data JSON file */
};

/*! \brief Holds information about the UAT receiver */
struct receiver_info {
	const char *version_string;			/*!< A string descibing the version of dump978 used to produce the UAT data */
	uint32_t refresh_interval;			/*!< The interval, in milliseconds, at which new data is made available */
	uint32_t history;					/*!< History information (???) \todo Document this properly! */
};


/*! \brief An array of aircraft data structures to hold information about all current aircraft */
static struct aircraft *aircraft_list;

/*! \brief A structure to hold the JSON file path and file name information */
static struct json_file_info g_json_file_info;

/*! \brief A structure to hold information about the UAT receiver */
static struct receiver_info g_receiver_info;

/*! \brief The current time */
static time_t NOW;

/*! \brief The count of UAT messages */
static uint32_t message_count;

#if 0
/*! \brief The directory in which the JSON data is to be stored */
static const char *json_dir;
#endif




static struct aircraft *find_aircraft(uint32_t address) {
	struct aircraft *a;

	for (a = aircraft_list; a; a = a->next) {
		if (a->address == address) {
			return (a);
		}
	}

	return (NULL);
}

static struct aircraft *find_or_create_aircraft(uint32_t address) {
	struct aircraft *a = find_aircraft(address);

	if (a) {
		return (a);
	}

	a = calloc(1, sizeof(*a));
	a->address = address;
	a->airground_state = AG_RESERVED;

	a->next = aircraft_list;
	aircraft_list = a;

	return (a);
}

static void expire_old_aircraft() {
	struct aircraft *a;
	struct aircraft **last;

	for (last = &aircraft_list, a = *last; a; a = *last) {
		if ((NOW - a->last_seen) > 300) {
			*last = a->next;
			free(a);
		} else {
			last = &a->next;
		}
	}
}

static void process_mdb(struct uat_adsb_mdb *mdb) {
	struct aircraft *a;
	uint32_t addr;

	++message_count;

	switch (mdb->address_qualifier) {
		case AQ_ADSB_ICAO:
		case AQ_TISB_ICAO:
			addr = mdb->address;
			break;

		default:
			addr = (mdb->address | NON_ICAO_ADDRESS);
			break;
	}

	a = find_or_create_aircraft(addr);
	a->last_seen = NOW;
	++a->messages;

	/* copy state into aircraft */
	if (mdb->airground_state != AG_RESERVED) {
		a->airground_state = mdb->airground_state;
	}

	if (mdb->position_valid) {
		a->position_valid = 1;
		a->lat = mdb->lat;
		a->lon = mdb->lon;
		a->last_seen_pos = NOW;
	}

	if (mdb->altitude_type != ALT_INVALID) {
		a->altitude_valid = 1;
		a->altitude = mdb->altitude;
	}

	if (mdb->track_type != TT_INVALID) {
		a->track_valid = 1;
		a->track = mdb->track;
	}

	if (mdb->speed_valid) {
		a->speed_valid = 1;
		a->speed = mdb->speed;
	}

	if (mdb->vert_rate_source != ALT_INVALID) {
		a->vert_rate_valid = 1;
		a->vert_rate = mdb->vert_rate;
	}

	if (mdb->callsign_type == CS_CALLSIGN) {
		strcpy(a->callsign, mdb->callsign);
	} else if (mdb->callsign_type == CS_SQUAWK) {
		strcpy(a->squawk, mdb->callsign);
	}

	if (mdb->sec_altitude_type != ALT_INVALID) {
		/* only use secondary if no primary is available */
		if (!a->altitude_valid || (mdb->altitude_type == ALT_INVALID)) {
			a->altitude_valid = 1;
			a->altitude = mdb->sec_altitude;
		}
	}
}


/**
 * \brief Write JSON-formatted data about the receiver to the JSON data directory
 *
 * \param[in]	dir		The path to the directory in which the JSON file should be placed
 *
 * \return An integer value, indicating whether the file was successfully written.
 * \retval	0	An error occurred while trying to write the file
 * \retval	1	The file was successfully written
 */
static int write_receiver_json(const char *dir) {
	char path[PATH_MAX];
	char path_new[PATH_MAX];
	FILE *f;

	g_receiver_info.version_string = "dump978-uat2json";
	g_receiver_info.refresh_interval = 1000;
	g_receiver_info.history = 0;

	if ((snprintf(path, PATH_MAX, "%s/receiver.json", dir) >= PATH_MAX) || (snprintf(path_new, PATH_MAX, "%s/receiver.json.new", dir) >= PATH_MAX)) {
		fprintf(stderr, "write_receiver_json: path too long\n");
		return (0);
	}

	if (!(f = fopen(path_new, "w"))) {
		fprintf(stderr, "fopen(%s): %m\n", path_new);
		return (0);
	}

#if 0
	fprintf(f, "{\n  \"version\" : \"dump978-uat2json\",\n  \"refresh\" : 1000,\n  \"history\" : 0\n}\n");
#else
	fprintf(f, "{\n");
	fprintf(f, "  \"version\" : \"%s\",\n", g_receiver_info.version_string);
	fprintf(f, "  \"refresh\" : \"%ul\",\n", g_receiver_info.refresh_interval);
	fprintf(f, "  \"history\" : \"%ul\",\n", g_receiver_info.history);
	fprintf(f, "}\n");
#endif
	fclose(f);

	if (rename(path_new, path) < 0) {
		fprintf(stderr, "rename(%s,%s): %m\n", path_new, path);
		return (0);
	}

	return (1);
}

static int write_aircraft_json(const char *dir) {
	char path[PATH_MAX];
	char path_new[PATH_MAX];
	FILE *f;
	struct aircraft *a;

	if ((snprintf(path, PATH_MAX, "%s/aircraft.json", dir) >= PATH_MAX) || (snprintf(path_new, PATH_MAX, "%s/aircraft.json.new", dir) >= PATH_MAX)) {
		fprintf(stderr, "write_aircraft_json: path too long\n");
		return 0;
	}

	if (!(f = fopen(path_new, "w"))) {
		fprintf(stderr, "fopen(%s): %m\n", path_new);
		return 0;
	}

	fprintf(f, "{\n");
	fprintf(f, "  \"now\" : %u,\n", (unsigned)NOW);
	fprintf(f, "  \"messages\" : %u,\n", message_count);
	fprintf(f, "  \"aircraft\" : [\n");


	for (a = aircraft_list; a; a = a->next) {
		if (a != aircraft_list)
			fprintf(f, ",\n");
		fprintf(f, "    {\"hex\":\"%s%06X\"", (a->address & NON_ICAO_ADDRESS) ? "~" : "", (a->address & 0xFFFFFF));

		if (a->squawk[0])
			fprintf(f, ",\"squawk\":\"%s\"", a->squawk);
		if (a->callsign[0])
			fprintf(f, ",\"flight\":\"%s\"", a->callsign);
		if (a->position_valid)
			fprintf(f, ",\"lat\":%.6f,\"lon\":%.6f,\"seen_pos\":%u", a->lat, a->lon, (unsigned)(NOW - a->last_seen_pos));
		if (a->altitude_valid)
			fprintf(f, ",\"altitude\":%d", a->altitude);
		if (a->vert_rate_valid)
			fprintf(f, ",\"vert_rate\":%d", a->vert_rate);
		if (a->track_valid)
			fprintf(f, ",\"track\":%u", a->track);
		if (a->speed_valid)
			fprintf(f, ",\"speed\":%u", a->speed);
		fprintf(f, ",\"messages\":%u,\"seen\":%u,\"rssi\":0}", a->messages, (unsigned)(NOW - a->last_seen));
	}

	fprintf(f, "\n  ]\n}\n");
	fclose(f);

	if (rename(path_new, path) < 0) {
		fprintf(stderr, "rename(%s,%s): %m\n", path_new, path);
		return 0;
	}

	return 1;
}

static void periodic_work() {
	static time_t next_write;

	if (NOW >= next_write) {
		expire_old_aircraft();
		write_aircraft_json(g_json_file_info.json_data_path);
		next_write = NOW + 1;
	}
}

static void handle_frame(frame_type_t type, uint8_t *frame, int len, void *extra) {
	struct uat_adsb_mdb mdb;

	if (type != UAT_DOWNLINK) {
		return;
	}

	if (len == SHORT_FRAME_DATA_BYTES) {
		if ((frame[0] >> 3) != 0) {
			fprintf(stderr, "short frame with non-zero type\n");
			return;
		}
	} else if (len == LONG_FRAME_DATA_BYTES) {
		if ((frame[0] >> 3) == 0) {
			fprintf(stderr, "long frame with zero type\n");
			return;
		}
	} else {
		fprintf(stderr, "odd frame size: %d\n", len);
		return;
	}

	uat_decode_adsb_mdb(frame, &mdb);
#if 0
	uat_display_adsb_mdb(&mdb, stdout);
#endif
	process_mdb(&mdb);
}

static void read_loop() {
	struct dump978_reader *reader;

	reader = dump978_reader_new(0, 1);
	if (!reader) {
		perror("dump978_reader_new");
		return;
	}

	while (1) {
		fd_set readset;
		fd_set writeset;
		fd_set excset;
		struct timeval timeout;
		int framecount;

		FD_ZERO(&readset);
		FD_ZERO(&writeset);
		FD_ZERO(&excset);
		FD_SET(0, &readset);
		FD_SET(0, &excset);
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;

		select(1, &readset, &writeset, &excset, &timeout);

		NOW = time(NULL);
		framecount = dump978_read_frames(reader, handle_frame, NULL);

		if (framecount == 0) {
			break;
		}

		if ((framecount < 0) && (errno != EAGAIN) && (errno != EINTR) && (errno != EWOULDBLOCK)) {
			perror("dump978_read_frames");
			break;
		}

		periodic_work();
	}

	dump978_reader_free(reader);
}


static void usage(char *argv0) {
	fprintf(stderr, "USAGE: %s [directory]\n\n", argv0);

	fprintf(stderr, "Reads UAT messages from stdin.\n");
	fprintf(stderr, "Periodically writes aircraft state to [directory]/aircraft.json\n");
	fprintf(stderr, "Additionally writes [directory]/receiver.json once, on startup.\n\n");
}


int main(int argc, char *argv[]) {
	if (argc < 2) {
#if 0
		fprintf(stderr,
				"Syntax: %s <dir>\n"
				"\n"
				"Reads UAT messages on stdin.\n"
				"Periodically writes aircraft state to <dir>/aircraft.json\n"
				"Also writes <dir>/receiver.json once on startup\n",
				argv[0]);
#else
		usage(argv[0]);
#endif

		return (1);
	}

#if 0
	json_dir = argv[1];
#else
	g_json_file_info.json_data_path = argv[1];
	g_json_file_info.aircraft_info_name = "aircraft.json";
	g_json_file_info.aircraft_info_name_new = "aircraft.json.new";
	g_json_file_info.receiver_info_name = "receiver.json";
	g_json_file_info.receiver_info_name_new = "receiver.json.new";
#endif

	if (!write_receiver_json(g_json_file_info.json_data_path)) {
		fprintf(stderr, "Failed to write %s/%s -- check permissions?\n", g_json_file_info.json_data_path, g_json_file_info.receiver_info_name);

		return (1);
	}

	read_loop();
	write_aircraft_json(g_json_file_info.json_data_path);

	return (0);
}
