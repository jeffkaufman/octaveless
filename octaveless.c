/* Derived from paex_sine.c by Jeff Kaufman 2012
 *
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com/
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however,
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also
 * requested that these non-binding requests be included along with the
 * license above.
 */
#include <stdio.h>
#include <math.h>
#include <termios.h>
#include "portaudio.h"
#include "definitions.h"

#define DECAY (100000)
#define SWITCH_PERIOD (1000)
#define NUM_SECONDS   (2)
#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (64)

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

typedef struct
{
  unsigned int phase;
  char message[20];
}
paTestData;

#define sine(i,F) ((float) sin( (((double)(i)*(double)(F))/SAMPLE_RATE) * M_PI * 2. ))

int target_note;

float freq(float note)
{
  return 440*(pow(2, (note-69.0)/12));
}

float intensity(float note, int octave)
{
  float bass_n = fmod(note, 12);

  switch(octave) {
  case 0:
    return 0.00 + 0.01*bass_n;
  case 1:
    return 0.12 + 0.01*bass_n;
  case 2:
    return 0.24 + 0.01*bass_n;
  case 3:
    return 0.32 - 0.01*bass_n;
  case 4:
    return 0.22 - 0.01*bass_n;
  case 5:
    return 0.10 - 0.01*bass_n;
  }
  return 0; // never reached
}

float synth(float phase, float note) {
  return pow(2,(sine(phase, note)));
}

float sample_val(float note, unsigned int phase)
{
  note = fmod(note, 12);

  if (phase % 1000 == 0) {
    printf("%.2f x %.2f, %.2f x %.2f, %.2f x %.2f, %.2f x %.2f, "
           "%.2f x %.2f, %.2f x %.2f\n", 
           intensity(note, 0), freq(note + 12*0),
           intensity(note, 1), freq(note + 12*1),
           intensity(note, 2), freq(note + 12*2),
           intensity(note, 3), freq(note + 12*3),
           intensity(note, 4), freq(note + 12*4),
           intensity(note, 5), freq(note + 12*5));
  }

  return log2(synth(phase, freq(note + 12*0)) * intensity(note, 0) +
              synth(phase, freq(note + 12*1)) * intensity(note, 1) +
              synth(phase, freq(note + 12*2)) * intensity(note, 2) +
              synth(phase, freq(note + 12*3)) * intensity(note, 3) +
              synth(phase, freq(note + 12*4)) * intensity(note, 4) +
              synth(phase, freq(note + 12*5)) * intensity(note, 5));
}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int patestCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;
    unsigned long i;

    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) inputBuffer;

    for( i=0; i<framesPerBuffer; i++ )
    {
      *out++ = sample_val(target_note, data->phase);
      data->phase += 1;
    }

    return paContinue;
}

/*
 * This routine is called by portaudio when playback is done.
 */
static void StreamFinished( void* userData )
{
   paTestData *data = (paTestData *) userData;
   printf( "Stream Completed: %s\n", data->message );
}

/*******************************************************************/
int main(void);
int main(void)
{
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;
    paTestData data;

    printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n",
           SAMPLE_RATE, FRAMES_PER_BUFFER);

    data.phase = 0;

    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (outputParameters.device == paNoDevice) {
      fprintf(stderr,"Error: No default output device.\n");
      goto error;
    }
    outputParameters.channelCount = 1;       /* mono output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &stream,
              NULL, /* no input */
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              &data );
    if( err != paNoError ) goto error;

    sprintf( data.message, "No Message" );
    err = Pa_SetStreamFinishedCallback( stream, &StreamFinished );
    if( err != paNoError ) goto error;

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    /* set up unbuffered reading */
    struct termios tio;
    tcgetattr(1,&tio);
    tio.c_lflag &=(~ICANON & ~ECHO);
    tcsetattr(1,TCSANOW,&tio);


    while(1) {
      switch (getc(stdin)) {
      case 'A':
        target_note = 69;
        break;
      case 'b':
        target_note = 70;
        break;
      case 'B':
        target_note = 71;
        break;
      case 'C':
        target_note = 72;
        break;
      case 'd':
        target_note = 73;
        break;
      case 'D':
        target_note = 74;
        break;
      case 'e':
        target_note = 75;
        break;
      case 'E':
        target_note = 76;
        break;
      case 'F':
        target_note = 77;
        break;
      case 'g':
        target_note = 78;
        break;
      case 'G':
        target_note = 79;
        break;
      case 'a':
        target_note = 80;
        break;
      }
      
    }

    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;

    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;

    Pa_Terminate();
    printf("Test finished.\n");

    return err;
error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}
