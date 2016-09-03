/**
 * SimpleSMIL a very small image player for SMIL image sequences
 * code based on the xmlReader example by Daniel Veillard
 */

#include <stdio.h>
#include <unistd.h>
#include <libxml/xmlreader.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "shapes.h"

enum mediatype { MEDIA_IMG = 0, MEDIA_VIDEO };

struct media {
	enum mediatype type;
	const xmlChar *src;
	const xmlChar *alt;
	int x, y;
	int width, height;
	struct timeval start;
	int dur;
	VGImage img;
	pthread_t pth_player;
	/* OMXPlayer player; */
};

int width, height;		// width and Height of the root window
pthread_t pth_preroll = 0;	// preroll thread
pthread_t pth_video = 0;	// video   thread

struct media *current = NULL;
struct media *next = NULL;

int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y) {
	/* Perform the carry for the later subtraction
	 * by updating y.
	 */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	 * tv_usec is certainly positive.
	 */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

void *videoThreadFunc(void *arg) {
#if 0
	struct media *media = (struct media *) arg;
	media->player->play();
	while (!media->player->m_stop) {
		/* theoretically we could check up to timing */
		media->player->playloop();
	}
#endif

	return NULL;
}

/**
 * processNode:
 * @reader: the xmlReader
 *
 * Dump information about the current node
 */
static void processNode(xmlTextReaderPtr reader) {
	const xmlChar *name = xmlTextReaderConstName(reader);

	/* preload the next media */
	if (xmlStrncmp(name, (const xmlChar *)"img", 3) == 0 || xmlStrncmp(name, (const xmlChar *)"video", 5) == 0) {
		const xmlChar *src = xmlTextReaderGetAttribute(reader, (const xmlChar *)"src");

		/* Check if the file exists */
		if (access((const char *)src, R_OK) == -1) return;

		next = (struct media *)malloc(sizeof(struct media));

		next->alt = xmlTextReaderGetAttribute(reader, (const xmlChar *)"alt");
		next->src = src;
		next->width = width;
		next->height = height;
		next->x = 0;
		next->y = 0;

		if (xmlStrncmp(name, (const xmlChar *)"img", 3) == 0) {
			const xmlChar *durs;
			int dur = 0;

			next->type = MEDIA_IMG;

			durs = xmlTextReaderGetAttribute(reader, (const xmlChar *)"dur");
			sscanf((const char *)durs, "%d", &next->dur);
			if (next->dur == 0) {
				free (next);
				return;
			}

			fprintf(stderr, "Found an image: %s (%d)\n", (const char *)next->alt, dur);
			fprintf(stderr, "Decoding the upcoming image\n");
			next->img = DecodeImage(next->width, next->height, (char *)next->src);
			if (next->img == VG_INVALID_HANDLE) {
				free (next);
				return;
			}

		} else
		if (xmlStrncmp(name, (const xmlChar *)"video", 3) == 0) {
			next->type = MEDIA_VIDEO;
			fprintf(stderr, "Prerolling the upcoming video\n");
#if 0			
			media->player->open(media->src);
#endif			
		}

		/* preloading done, how long do we have to wait? */
		if (current != NULL) {
			if (current->type == MEDIA_IMG) {
				if (current->dur > 0) {
					int remaining;
					struct timeval elapsed, now;
					gettimeofday(&now, NULL);
					timeval_subtract(&elapsed, &now, &(current->start));
					remaining = current->dur - elapsed.tv_sec;
					fprintf(stderr, "Calculating remaining sleep of the last image: %d\n", remaining);
					if (remaining > 0 && current->dur != 0) {
						printf("\n\n - Sleeping %ds - \n\n", remaining);
						sleep(remaining);
					}
				}
			} else
			 if (current->type == MEDIA_VIDEO) {
				pthread_join(current->pth_player, NULL);
			}
		}

		if (next->type == MEDIA_IMG) {
			fprintf(stderr, "Onscreen Image: %s\n", next->src);
			Start(next->width, next->height);
			DisplayImage(next->x, next->y, next->width, next->height, next->img);
			End();
			gettimeofday(&next->start, NULL);
		} else
		if (next->type == MEDIA_VIDEO) {
			fprintf(stderr, "Onscreen Video: %s\n", next->src);
			pthread_create(&next->pth_player, NULL, videoThreadFunc, next);
		}

		if (current->type == MEDIA_VIDEO) {
#if 0
			current->player->close();
#endif			
		}

		/* test if line below solves memory leak */
		if (current) {
			free(current);
		}
		current = next;

	}
}

/**
 * streamFile:
 * @filename: the file name to parse
 *
 * Parse and print information about an XML file.
 */
static void streamFile(const char *filename) {
	xmlTextReaderPtr reader;
	int ret;

	reader = xmlReaderForFile(filename, NULL, 0);
	if (reader != NULL) {
		ret = xmlTextReaderRead(reader);
		while (ret == 1) {
			processNode(reader);
			ret = xmlTextReaderRead(reader);
		}
		xmlFreeTextReader(reader);
		if (ret != 0) {
			fprintf(stderr, "%s : failed to parse\n", filename);
			sleep(5);
		}
	} else {
		fprintf(stderr, "Unable to open %s\n", filename);
		sleep(5);
	}
}

int main(int argc, char **argv) {
	/*
	 * this initialize the library and check potential ABI mismatches
	 * between the version it was compiled for and the actual shared
	 * library used.
	 */
	LIBXML_TEST_VERSION if (chdir("/mnt/media") == -1) {
		fprintf(stderr, "/home/tv does not exist");
		return -1;
	}

	init(&width, &height);
	Background(0, 0, 0);

	while (1) {
		streamFile("broadcast.smil");
	}

	/*
	 * Cleanup function for the XML library.
	 */
	xmlCleanupParser();

	/*
	 * this is to debug memory for regression tests
	 * xmlMemoryDump();
	 */
	finish();
	return (0);
}
