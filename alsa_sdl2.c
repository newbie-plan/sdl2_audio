#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <unistd.h>


typedef struct audio_queue {
	char *data;
	int data_len;
	struct audio_queue *next;
}AudioQueue;

typedef struct audio_state {
	AudioQueue *first;
	AudioQueue *last;
	AudioQueue *q;
	uint8_t *audio_buf;
	unsigned int audio_buf_size;
	unsigned int audio_buf_index;
}AudioState;


void free_list(AudioQueue *first)
{
	AudioQueue *k = NULL;

	if (first != NULL)
	{
		k = first;
		first = first->next;
		if (k->data != NULL)
			free(k->data);
		free(k);
	}
}

void fill_audio(void *udata, Uint8 *stream, int len)
{
	int len1 = 0;
	AudioState *as = udata;

	SDL_memset(stream, 0, len);

	while (len > 0)
	{
		if (as->first == NULL)
			return;

		if (as->audio_buf_index >= as->audio_buf_size)
		{
			if (as->q != NULL)
			{
				if (as->q->data != NULL)
					free(as->q->data);
				free(as->q);
			}

			as->q = as->first;
			as->audio_buf = as->q->data;
			as->audio_buf_size = as->q->data_len;
			as->audio_buf_index = 0;

			as->first = as->first->next;
			if (as->first == NULL)
				as->last = NULL;
		}

		len1 = as->audio_buf_size - as->audio_buf_index;
		if (len1 > len)
			len1 = len;

		if (as->audio_buf != NULL)
			SDL_MixAudio(stream, as->audio_buf + as->audio_buf_index, len1, SDL_MIX_MAXVOLUME);

		len -= len1;
		stream += len1;
		as->audio_buf_index += len1;
	}
}

void alsa_sdl2(const char *input_file)
{
	FILE *fp;
	int samples = 1024;
	int sample_rate = 44100;
	int channels = 2;
	int buf_size = samples * channels * 2;
	AudioState as = {0};
	SDL_AudioSpec wanted_spec = {0};

	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		fprintf(stderr, "SDL_Init failed\n");
		goto ret1;
	}

	wanted_spec.freq = sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = channels;
	wanted_spec.silence = 0;
	wanted_spec.samples = samples;
	wanted_spec.callback = fill_audio;
	wanted_spec.userdata = &as;

	if (SDL_OpenAudio(&wanted_spec, NULL) < 0)
	{
		fprintf(stderr, "SDL_OpenAudio failed.\n");
		goto ret1;
	}

	if ((fp = fopen(input_file, "rb")) == NULL)
	{
		fprintf(stderr, "fopen failed.\n");
		goto ret1;
	}

	SDL_PauseAudio(0);

	while (1)
	{
		char *audio_buf = NULL;
		AudioQueue *p = NULL;

		if ((audio_buf = (char *)malloc(buf_size + 1)) == NULL)
		{
			fprintf(stderr, "malloc for audio_buf failed.\n");
			goto ret2;
		}
		memset(audio_buf, 0, buf_size + 1);
		if (fread(audio_buf, 1, buf_size, fp) != buf_size)
		{
			printf("The end...\n");
			break;
		}

		/*audio queue*/
		if ((p = (AudioQueue *)malloc(sizeof(AudioQueue))) == NULL)
		{
			fprintf(stderr, "malloc for p failed.\n");
			goto ret2;
		}
		memset(p, 0, sizeof(AudioQueue));

		p->data = audio_buf;
		p->data_len = buf_size;

		if (as.last == NULL)
			as.first = p;
		else
			as.last->next = p;
		as.last = p;
	}

	/*wait for play finished*/
	while (as.first != NULL)
		sleep(1);

	printf("Play finished...\n");

ret2:
	free_list(as.first);
	fclose(fp);
ret1:
	return;
}

int main(int argc, const char *argv[])
{
	if (argc < 2)
	{
		printf("Usage:<input *.pcm file>\n");
		return -1;
	}

	alsa_sdl2(argv[1]);

	return 0;
}
