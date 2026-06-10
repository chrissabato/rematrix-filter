#include <obs-module.h>
#include <media-io/audio-math.h>
#include <math.h>
#include <atomic>
#include <string>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("rematrix-filter", "en-US")

#define MT_ obs_module_text

#ifndef AUDIO_OUTPUT_FRAMES
#define AUDIO_OUTPUT_FRAMES 1024
#endif
#define MAX_AUDIO_SIZE (AUDIO_OUTPUT_FRAMES * sizeof(float))

#define MATRIX_DB_MIN  -96.0
#define MATRIX_DB_MAX   20.0
#define MATRIX_DB_STEP   0.1
#define SILENCE_DB     -96.0

struct rematrix_data {
	obs_source_t *context;
	size_t        channels;
	// matrix[out][in]: linear gain for mixing input 'in' into output 'out'
	std::atomic<float> matrix[MAX_AV_PLANES][MAX_AV_PLANES];
	uint8_t *tmpbuffer[MAX_AV_PLANES];
};

/*****************************************************************************/
static const char *rematrix_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return MT_("Rematrix");
}

/*****************************************************************************/
static std::string mix_key(size_t out, size_t in)
{
	return "mix_" + std::to_string(out) + "_" + std::to_string(in);
}

/*****************************************************************************/
static void rematrix_update(void *data, obs_data_t *settings)
{
	struct rematrix_data *rematrix = (struct rematrix_data *)data;
	rematrix->channels = audio_output_get_channels(obs_get_audio());

	for (size_t out = 0; out < MAX_AV_PLANES; out++) {
		for (size_t in = 0; in < MAX_AV_PLANES; in++) {
			double db = obs_data_get_double(
				settings, mix_key(out, in).c_str());
			float linear = (db <= SILENCE_DB)
					       ? 0.0f
					       : (float)db_to_mul((float)db);
			rematrix->matrix[out][in].store(linear);
		}
	}
}

/*****************************************************************************/
static void *rematrix_create(obs_data_t *settings, obs_source_t *filter)
{
	struct rematrix_data *rematrix =
		(struct rematrix_data *)bzalloc(sizeof(*rematrix));
	rematrix->context = filter;
	rematrix_update(rematrix, settings);

	for (size_t i = 0; i < rematrix->channels; i++)
		rematrix->tmpbuffer[i] = (uint8_t *)bzalloc(MAX_AUDIO_SIZE);

	return rematrix;
}

/*****************************************************************************/
static void rematrix_destroy(void *data)
{
	struct rematrix_data *rematrix = (struct rematrix_data *)data;
	for (size_t i = 0; i < rematrix->channels; i++) {
		if (rematrix->tmpbuffer[i])
			bfree(rematrix->tmpbuffer[i]);
	}
	bfree(rematrix);
}

/*****************************************************************************/
static struct obs_audio_data *rematrix_filter_audio(void *data,
						    struct obs_audio_data *audio)
{
	struct rematrix_data *rematrix = (struct rematrix_data *)data;
	const size_t channels = rematrix->channels;
	float **fdata = (float **)audio->data;
	float **fout  = (float **)rematrix->tmpbuffer;

	// Snapshot matrix once to avoid repeated atomic loads in inner loop
	float matrix[MAX_AV_PLANES][MAX_AV_PLANES];
	for (size_t out = 0; out < channels; out++)
		for (size_t in = 0; in < channels; in++)
			matrix[out][in] = rematrix->matrix[out][in].load();

	uint32_t frames = audio->frames;
	for (size_t chunk = 0; chunk < frames; chunk += AUDIO_OUTPUT_FRAMES) {
		size_t n = frames - chunk;
		if (n > AUDIO_OUTPUT_FRAMES)
			n = AUDIO_OUTPUT_FRAMES;

		// Mix inputs into temp buffer
		for (size_t out = 0; out < channels; out++) {
			float *dst = (float *)fout[out];
			memset(dst, 0, n * sizeof(float));
			for (size_t in = 0; in < channels; in++) {
				float g = matrix[out][in];
				if (g == 0.0f || !fdata[in])
					continue;
				const float *src = fdata[in] + chunk;
				for (size_t s = 0; s < n; s++)
					dst[s] += src[s] * g;
			}
		}

		// Write mixed result back
		for (size_t out = 0; out < channels; out++) {
			if (fdata[out])
				memcpy(fdata[out] + chunk, fout[out],
				       n * sizeof(float));
		}
	}

	return audio;
}

/*****************************************************************************/
static void rematrix_defaults(obs_data_t *settings)
{
	for (size_t out = 0; out < MAX_AV_PLANES; out++) {
		for (size_t in = 0; in < MAX_AV_PLANES; in++) {
			// Default: identity matrix — each output passes its
			// own input at 0 dB, all cross-channel gains silent
			double db = (out == in) ? 0.0 : SILENCE_DB;
			obs_data_set_default_double(
				settings, mix_key(out, in).c_str(), db);
		}
	}
}

/*****************************************************************************/
static obs_properties_t *rematrix_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();
	size_t channels = audio_output_get_channels(obs_get_audio());

	for (size_t out = 0; out < channels; out++) {
		char group_name[32];
		snprintf(group_name, sizeof(group_name), "out_group_%zu", out);

		char group_label[64];
		snprintf(group_label, sizeof(group_label),
			 "Output Channel %zu", out + 1);

		obs_properties_t *group = obs_properties_create();

		for (size_t in = 0; in < channels; in++) {
			char label[64];
			snprintf(label, sizeof(label),
				 "← Input Channel %zu (dB)", in + 1);
			obs_properties_add_float_slider(
				group, mix_key(out, in).c_str(), label,
				MATRIX_DB_MIN, MATRIX_DB_MAX, MATRIX_DB_STEP);
		}

		obs_properties_add_group(props, group_name, group_label,
					 OBS_GROUP_NORMAL, group);
	}

	return props;
}

/*****************************************************************************/
bool obs_module_load(void)
{
	struct obs_source_info rematrixer_filter = {
		.id             = "rematrix_filter",
		.type           = OBS_SOURCE_TYPE_FILTER,
		.output_flags   = OBS_SOURCE_AUDIO,
		.get_name       = rematrix_name,
		.create         = rematrix_create,
		.destroy        = rematrix_destroy,
		.get_defaults   = rematrix_defaults,
		.get_properties = rematrix_properties,
		.update         = rematrix_update,
		.filter_audio   = rematrix_filter_audio,
	};

	obs_register_source(&rematrixer_filter);
	return true;
}
