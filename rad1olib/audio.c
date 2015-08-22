/*
audio output library for the rad1o 

Copyright (c) 2015 Hans-Werner Hilse <hwhilse@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <libopencm3/lpc43xx/dac.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/lpc43xx/ritimer.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>

#include <rad1olib/pins.h>

static uint16_t *audio;
static int audio_size;

static volatile int audio_head = 0;
static volatile int audio_pos = 0;
static volatile int audio_running = 0;

static volatile int audio_volume = 4;

static volatile int ritimer_val;

static vector_table_entry_t old_isr;

/*
 * get current volume setting (0..9)
 */
int audio_get_volume() {
	return audio_volume;
}

/*
 * set volume (0..9)
 *
 * volume setting 9 enables the amplifier
 */
void audio_set_volume(int volume) {
	audio_volume = volume;
	if(volume == 9) {
		OFF(MIC_AMP_DIS); // enable amp
	} else {
		ON(MIC_AMP_DIS); // disable amp
	}
}

/*
 * stop playing audio
 */
void audio_stop(void) {
    // stop interrupt handler
    RITIMER_CTRL &= ~(RITIMER_CTRL_RITEN(1));
    nvic_disable_irq(NVIC_RITIMER_IRQ);
    vector_table.irq[NVIC_RITIMER_IRQ] = old_isr;
    dac_set(0);
    ON(MIC_AMP_DIS); // disable amp
}

/*
 * return number of samples in sample buffer
 */
int audio_fill() {
    // wraparound
    if(audio_head < audio_pos) return audio_head + (audio_size - audio_pos);
    return audio_head - audio_pos;
}

/*
 * interrupt handler function
 */
static void SAMPLES_isr(void) {
    // check if it actually is an RITIMER interrupt
    if((RITIMER_CTRL | RITIMER_CTRL_RITINT(1)) == 0) {
        // nope, ignore it.
        return;
    }

    if(audio_running != 0 && audio_pos != audio_head) {
        // output sample
        dac_set((audio[audio_pos] * (audio_volume & 7)) >> 2);

        // advance counters
        audio_pos++;
        if(audio_pos == audio_size) audio_pos = 0;

        // set compval to new value (unconditionally, but
        // in fact, there shouldn't be many actual changes)
        RITIMER_COMPVAL = ritimer_val;
    }

    // reset interrupt flag
    RITIMER_CTRL |= RITIMER_CTRL_RITINT(1);
}

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
void audio_init(uint16_t *buf, int size, int rate, int timer_freq) {
    audio = buf;
    audio_size = size;

	// set up mic (but not just mic?) amp
	SETUPgout(MIC_AMP_DIS);
	ON(MIC_AMP_DIS); // disable amp
	// set up DAC
	dac_init(false); 

    audio_pos = 0;
    audio_head = 0;
    audio_running = 0;

    ritimer_val = timer_freq / rate;

    // timer setup:
    // save old interrupt handler:
    old_isr = vector_table.irq[NVIC_RITIMER_IRQ];

    // set up RITimer
    RITIMER_MASK = 0;
    RITIMER_COUNTER = 0;
    RITIMER_CTRL |= RITIMER_CTRL_RITEN(1) | RITIMER_CTRL_RITENCLR(1);
    RITIMER_COMPVAL = ritimer_val;

    // set new interrupt handler
    vector_table.irq[NVIC_RITIMER_IRQ] = SAMPLES_isr;

    // enable ritimer interrupt handling in NVIC
    nvic_enable_irq(NVIC_RITIMER_IRQ);
    nvic_set_priority(NVIC_RITIMER_IRQ, 1);
}

/*
 * start playing audio immediately
 */
void audio_play() {
    audio_running = 1;
}

/*
 * start playing audio if the sample buffer is at least half filled
 */
int audio_play_when_half_full() {
    if(!audio_running && audio_fill()*2 >= audio_size) audio_running = 1;
    return audio_running;
}

/*
 * push a sample into the buffer
 */
void audio_push(uint16_t sample) {
    audio[audio_head++] = sample;
    if(audio_head == audio_size)
        audio_head = 0;
}

/*
 * issue this to adapt timers to tune replay sample rate
 *
 * assumes that the buffer ought to be half filled when called.
 * uses that to tune the timer value.
 *
 * call this any N samples to adapt playback sample rate
 * or not at all if you don't need it.
 */
void audio_tune_speed() {
    int corr = audio_fill() - (audio_size / 2);
    if(corr < -(audio_size/4)) ritimer_val++;
    if(corr > audio_size/4) {
        ritimer_val--;
        if(ritimer_val <= 0) ritimer_val = 1;
    }
}
