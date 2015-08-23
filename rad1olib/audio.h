#ifndef __AUDIO_H__
#define __AUDIO_H__
/*
 * set up audio output
 *
 * interrupt handler will be called after this is executed.
 * however, playback will not start yet. For that, call
 * audio_play().
 *
 * buf         buffer for samples
 * size        size of the buffer in number of samples
 * rate        sample rate
 * timer_freq  assume this to be the frequency of the ritimer clock
 */
void audio_init(uint16_t *buf, int size, int rate, int timer_freq);
/*
 * stop playing audio
 */
void audio_stop(void);
/*
 * return number of samples in sample buffer
 */
int audio_fill();
/*
 * start playing audio immediately
 */
void audio_play();
/*
 * start playing audio if the sample buffer is at least half filled
 *
 * returns 1 when audio is playing, 0 otherwise
 */
int audio_play_when_half_full();
/*
 * push a sample into the buffer
 */
void audio_push(uint16_t sample);
/*
 * issue this to adapt timers to tune replay sample rate
 *
 * assumes that the buffer ought to be half filled when called.
 * uses that to tune the timer value.
 *
 * call this any N samples to adapt playback sample rate
 * or not at all if you don't need it.
 */
void audio_tune_speed();
/*
 * set volume (0..9)
 *
 * volume setting 9 enables the amplifier
 */
void audio_set_volume(int volume);
/*
 * get current volume setting (0..9)
 */
int audio_get_volume();

#endif // __AUDIO_H__
