/*
 * copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of Libav.
 *
 * Libav is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Libav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Libav; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

// From http://code.haskell.org/~thielema/audiovideo-example/cbits/
// Adapted to version version 2.8.6-1ubuntu2 by Jan Newmarch

/**
 * @file
 * libavcodec API use example.
 *
 * @example libavcodec/api-example.c
 * Note that this library only handles codecs (mpeg, mpeg4, etc...),
 * not file formats (avi, vob, etc...). See library 'libavformat' for the
 * format handling
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_AV_CONFIG_H
#undef HAVE_AV_CONFIG_H
#endif

#include "libavcodec/avcodec.h"
#include <libavformat/avformat.h>

#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

void die(char *s) {
    fputs(s, stderr);
    exit(1);
}

/*
 * Audio decoding.
 */
static void audio_decode_example(AVFormatContext* container,
				 const char *outfilename, const char *filename)
{
    AVCodec *codec;
    AVCodecContext *context = NULL;
    int len;
    FILE *f, *outfile;
    uint8_t inbuf[AUDIO_INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];
    AVPacket avpkt;
    AVFrame *decoded_frame = NULL;
    int num_streams = 0;
    int sample_size = 0;

    av_init_packet(&avpkt);

    printf("Audio decoding\n");
    
    int stream_id = -1;
 
    // To find the first audio stream. This process may not be necessary
    // if you can gurarantee that the container contains only the desired
    // audio stream
    int i;
    for (i = 0; i < container->nb_streams; i++) {
        if (container->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            stream_id = i;
            break;
        }
    }
    
    /* find the appropriate audio decoder */
    AVCodecContext* codec_context = container->streams[stream_id]->codec;
    codec = avcodec_find_decoder(codec_context->codec_id);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }

    context = avcodec_alloc_context3(codec);;

    /* open it */
    if (avcodec_open2(context, codec, NULL) < 0) {
        fprintf(stderr, "could not open codec\n");
        exit(1);
    }

    f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "could not open %s\n", filename);
        exit(1);
    }
    outfile = fopen(outfilename, "wb");
    if (!outfile) {
        av_free(context);
        exit(1);
    }
    
    /* decode until eof */
    avpkt.data = inbuf;
    avpkt.size = fread(inbuf, 1, AUDIO_INBUF_SIZE, f);
    
    while (avpkt.size > 0) {
        int got_frame = 0;

        if (!decoded_frame) {
            if (!(decoded_frame = av_frame_alloc())) {
                fprintf(stderr, "out of memory\n");
                exit(1);
            }
        } else {
            av_frame_unref(decoded_frame);
	}
	printf("Stream idx %d\n", avpkt.stream_index);
	
        len = avcodec_decode_audio4(context, decoded_frame, &got_frame, &avpkt);
        if (len < 0) {
            fprintf(stderr, "Error while decoding\n");
            exit(1);
        }
        if (got_frame) {
	    printf("Decoded frame nb_samples %d, format %d\n", 
		   decoded_frame->nb_samples,
		   decoded_frame->format);
	    if (decoded_frame->data[1] != NULL)
	        printf("Data[1] not null\n");
	    else
		printf("Data[1] is null\n");
            /* if a frame has been decoded, output it */
            int data_size = av_samples_get_buffer_size(NULL, context->channels,
                                                       decoded_frame->nb_samples,
                                                       context->sample_fmt, 1);
	    // first time: count the number of  planar streams
	    if (num_streams == 0) {
		while (num_streams < AV_NUM_DATA_POINTERS &&
		       decoded_frame->data[num_streams] != NULL) 
		    num_streams++;
		printf("Number of streams %d\n", num_streams);
	    }

	    // first time: set sample_size from 0 to e.g 2 for 16-bit data
	    if (sample_size == 0) {
		sample_size = 
		    data_size / (num_streams * decoded_frame->nb_samples);
	    }

	    int m, n;
	    for (n = 0; n < decoded_frame->nb_samples; n++) {
		// interleave the samples from the planar streams
		for (m = 0; m < num_streams; m++) {
		    fwrite(&decoded_frame->data[m][n*sample_size], 
			   1, sample_size, outfile);
		}
	    }
        }
        avpkt.size -= len;
        avpkt.data += len;
        if (avpkt.size < AUDIO_REFILL_THRESH) {
            /* Refill the input buffer, to avoid trying to decode
             * incomplete frames. Instead of this, one could also use
             * a parser, or use a proper container format through
             * libavformat. */
            memmove(inbuf, avpkt.data, avpkt.size);
            avpkt.data = inbuf;
            len = fread(avpkt.data + avpkt.size, 1,
                        AUDIO_INBUF_SIZE - avpkt.size, f);
            if (len > 0)
                avpkt.size += len;
        }
    }

    fclose(outfile);
    fclose(f);

    avcodec_close(context);
    av_free(context);
    av_free(decoded_frame);
}

int main(int argc, char **argv)
{
    const char *filename = "Beethoven_Fr_Elise.mp3";
    AVFormatContext *pFormatCtx = NULL;

    if (argc == 2) {
        filename = argv[1];
    }

    // Register all formats and codecs
    av_register_all();
    if(avformat_open_input(&pFormatCtx, filename, NULL, NULL)!=0) {
	fprintf(stderr, "Can't get format of file %s\n", filename);
        return -1; // Couldn't open file
    }
    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx, NULL)<0)
	return -1; // Couldn't find stream information
    av_dump_format(pFormatCtx, 0, filename, 0);
    printf("Num streams %d\n", pFormatCtx->nb_streams);
    printf("Bit rate %d\n", pFormatCtx->bit_rate);
    audio_decode_example(pFormatCtx, "/tmp/test.sw", filename);

    return 0;
}
