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

#define ATTACK_RATE (0.001)
#define DECAY_RATE (0.0001)

#define NUM_SECONDS   (2)
#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (64)

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

#define DECAY 0
#define ATTACK 1


typedef struct
{
  unsigned int phase;
  char message[20];
  char states[12];
  float amplitudes[12];
}
paTestData;

#define sine(i,F) ((float) sin( (((double)(i)*(double)(F))/SAMPLE_RATE) * M_PI * 2. ))

float freq(float note)
{
  return 440*(pow(2, (note-69.0)/12));
}

float intensity(int note, int octave)
{
  int bass_n = note % 12;

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
  return sine(phase, note);
}

float sample_val(int note, unsigned int phase)
{

  float f;

  // f = freq(note+12);

  switch(note) {
  case 0:
    f = NOTE_0;
    break;
  case 1:
    f = NOTE_1;
    break;
  case 2:
    f = NOTE_2;
    break;
  case 3:
    f = NOTE_3;
    break;
  case 4:
    f = NOTE_4;
    break;
  case 5:
    f = NOTE_5;
    break;
  case 6:
    f = NOTE_6;
    break;
  case 7:
    f = NOTE_7;
    break;
  case 8:
    f = NOTE_8;
    break;
  case 9:
    f = NOTE_9;
    break;
  case 10:
    f = NOTE_10;
    break;
  case 11:
    f = NOTE_11;
    break;
  }

  return (synth(phase, f) * intensity(note, 0) +
          synth(phase, f*2) * intensity(note, 1) +
          synth(phase, f*4) * intensity(note, 2) +
          synth(phase, f*8) * intensity(note, 3) +
          synth(phase, f*16) * intensity(note, 4) +
          synth(phase, f*32) * intensity(note, 5));
}

float sample_combined(paTestData *data) {
  int i;
  float sum = 0;
  for (i = 0 ; i < 12 ; i++) {
    sum += sample_val(i, data->phase) * data->amplitudes[i];
    
    if (data->states[i] == ATTACK) {
      data->amplitudes[i] += ATTACK_RATE;
      if (data->amplitudes[i] > .9) {
        // printf("%d: beginning decay, amp=%.5f\n", i, data->amplitudes[i]);
        data->states[i] = DECAY; 
      }
    }
    else {
      data->amplitudes[i] -= DECAY_RATE;
      if (data->amplitudes[i] < 0) { data->amplitudes[i] = 0; }
    }
  }

  // sum = synth(data->phase, freq(69));

  return sum;
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
      *out++ = sample_combined(data);
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
    int i;

    printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n",
           SAMPLE_RATE, FRAMES_PER_BUFFER);

    data.phase = 0;
    for (i = 0 ; i < 12 ; i++) {
      data.amplitudes[i] = 0;
      data.states[i] = DECAY;
    }

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

    int pressed = -1;
    while(1) {
      switch (getc(stdin)) {
      case 'C':
        pressed = 0;
        break;
      case 'd':
        pressed = 1;
        break;
      case 'D':
        pressed = 2;
        break;
      case 'e':
        pressed = 3;
        break;
      case 'E':
        pressed = 4;
        break;
      case 'F':
        pressed = 5;
        break;
      case 'g':
        pressed = 6;
        break;
      case 'G':
        pressed = 7;
        break;
      case 'a':
        pressed = 8;
        break;
      case 'A':
        pressed = 9;
        break;
      case 'b':
        pressed = 10;
        break;
      case 'B':
        pressed = 11;
        break;
      }

      if (pressed != -1) {
        data.states[pressed] = ATTACK;
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
