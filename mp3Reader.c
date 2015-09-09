#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <stdlib.h>


void printAudioFrameInfo(const AVCodecContext* codecContext, const AVFrame* frame);
int getMetaAudio(char *f);

int main(int argc, char *argv[]) {
	
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	
	fp = fopen("/home/jonathan/Desktop/audioDSP/fileAccess/Paths.txt", "r");
	
	
	/*
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        //printf("Retrieved line of length %zu :\n", read);
        //printf("%s", line);
		   
		getMetaAudio(line); //Doing this gives an error in opening the file...
		   
    }
	*/
	
	
	//but this works fine
	getMetaAudio("/media/music/Blackmill - Miracle [2011]/01 - Miracle.mp3"); //place directory of song
	   

    fclose(fp);
	
	
	
	return 0;
}

void printAudioFrameInfo(const AVCodecContext* codecContext, const AVFrame* frame) {
	
    // See the following to know what data type (unsigned char, short, float, etc) to use to access the audio data:
    // http://ffmpeg.org/doxygen/trunk/samplefmt_8h.html#af9a51ca15301871723577c730b5865c5
    printf("Audio frame info:\n Sample count: %d\n", frame->nb_samples);
    printf("Channel count: %d\n", codecContext->channels);
    printf("Format: %s\n", av_get_sample_fmt_name(codecContext->sample_fmt));
	printf("Bytes per sample: %d\n", av_get_bytes_per_sample(codecContext->sample_fmt));
    //printf("Is planar? ", av_sample_fmt_is_planar(codecContext->sample_fmt), '\n');


    printf("frame->linesize[0] tells you the size (in bytes) of each plane\n");

    if (codecContext->channels > AV_NUM_DATA_POINTERS && av_sample_fmt_is_planar(codecContext->sample_fmt))
    {
        printf("The audio stream (and its frames) have too many channels to fit in\n"
               "frame->data. Therefore, to access the audio data, you need to use\n"
               "frame->extended_data to access the audio data. It's planar, so\n"
               "each channel is in a different element. That is:\n"
               "  frame->extended_data[0] has the data for channel 1\n"
               "  frame->extended_data[1] has the data for channel 2\n"
               "  etc.\n");
    }
    else
    {
        printf("Either the audio data is not planar, or there is enough room in\n"
               "frame->data to store all the channels, so you can either use\n"
               "frame->data or frame->extended_data to access the audio data (they\n"
               "should just point to the same data).\n");
    }

    printf("If the frame is planar, each channel is in a different element.\n"
           "That is:\n"
           "  frame->data[0]/frame->extended_data[0] has the data for channel 1\n"
           "  frame->data[1]/frame->extended_data[1] has the data for channel 2\n"
           "  etc.\n");

    printf("If the frame is packed (not planar), then all the data is in\n"
           "frame->data[0]/frame->extended_data[0] (kind of like how some\n"
           "image formats have RGB pixels packed together, rather than storing\n"
           " the red, green, and blue channels separately in different arrays.\n");
}

int getMetaAudio(char *f) {
	
	//printf(f);
	
	av_register_all();

    AVFrame *frame = av_frame_alloc();
	
	if (!frame)
    {
        printf("Error allocating the frame\n");
        return 1;
    }

    // you can change the file name "01 Push Me to the Floor.wav" to whatever the file is you're reading, like "myFile.ogg" or
    // "someFile.webm" and this should still work
    AVFormatContext *formatContext = NULL;
	
    if (avformat_open_input(&formatContext, f, NULL, NULL) != 0) //open the file
    {
        av_free(frame);
        printf("Error opening the file\n");
        return 1;
    }

    if (avformat_find_stream_info(formatContext, NULL) < 0) //get stream info
    {
        av_free(frame);
        avformat_close_input(&formatContext);
        printf("Error finding the stream info\n");
        return 1;
    }

    // Find the audio stream
    AVCodec* cdc;
    int streamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &cdc, 0);
	
    if (streamIndex < 0)
    {
        av_free(frame);
        avformat_close_input(&formatContext);
        printf("Could not find any audio stream in the file\n");
        return 1;
    }

    AVStream* audioStream = formatContext->streams[streamIndex];
	
	
    AVCodecContext* codecContext = audioStream->codec;
    codecContext->codec = cdc;

    if (avcodec_open2(codecContext, codecContext->codec, NULL) != 0) //open the codec
    {
        av_free(frame);
        avformat_close_input(&formatContext);
        printf("Couldn't open the context with the decoder\n");
        return 1;
    }

    printf("This stream has %d channels and a sample rate of %d Hz\n", codecContext->channels, codecContext->sample_rate);
    printf("The data is in the format %s\n", av_get_sample_fmt_name(codecContext->sample_fmt));
	
	
    AVPacket readingPacket;
    av_init_packet(&readingPacket);
	
	

    // Read the packets in a loop
    while (av_read_frame(formatContext, &readingPacket) >= 0)
    { 
		
        if (readingPacket.stream_index == audioStream->index)
        {
			//DECODE AUDIO
            AVPacket decodingPacket = readingPacket;

            // Audio packets can have multiple audio frames in a single packet
            while (decodingPacket.size > 0)
            {
                // Try to decode the packet into a frame
                // Some frames rely on multiple packets, so we have to make sure the frame is finished before
                // we can use it
                int gotFrame = 0;
                int result = avcodec_decode_audio4(codecContext, frame, &gotFrame, &decodingPacket);

                if (result >= 0 && gotFrame)
                {
                    decodingPacket.size -= result;
                    decodingPacket.data += result;

                    // We now have a fully decoded audio frame
                    //printAudioFrameInfo(codecContext, frame);   //PRINTS DECODED AUDIO FRAME
                }
                else
                {
                    decodingPacket.size = 0;
                    decodingPacket.data = NULL;
                }
            }
        }

        // You *must* call av_free_packet() after each call to av_read_frame() or else you'll leak memory
        av_free_packet(&readingPacket);
    }

    // Some codecs will cause frames to be buffered up in the decoding process. If the CODEC_CAP_DELAY flag
    // is set, there can be buffered up frames that need to be flushed, so we'll do that
    if (codecContext->codec->capabilities & CODEC_CAP_DELAY)
    {
        av_init_packet(&readingPacket);
        // Decode all the remaining frames in the buffer, until the end is reached
        int gotFrame = 0;
        while (avcodec_decode_audio4(codecContext, frame, &gotFrame, &readingPacket) >= 0 && gotFrame)
        {
            // We now have a fully decoded audio frame
            printAudioFrameInfo(codecContext, frame);
        }
    }
	
	
	int j;
	printf("Size: %ld\n", sizeof(frame->data[0]));
	printf("Size of datatype: %ld\n", sizeof(frame->data[0][0]));
	
	for(j = 0; j < 1000; j++) {  //This prints out the decoded signal. Not sure how to get the size of it though.
	
        printf("%d,", frame->extended_data[0][j]);
    }
	
	
    // Clean up!
    av_free(frame);
    avcodec_close(codecContext);
    avformat_close_input(&formatContext);
	
	return 0;
}




