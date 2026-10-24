// SPDX-License-Identifier: LGPL-2.1-or-later
// Host-only codec + OpenAL Soft loopback smoke test. Generates its own Ogg/Vorbis
// tone so the test does not depend on game assets or a copyrighted fixture.
#define AL_LIBTYPE_STATIC
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string &message)
{
	if (!condition)
		throw std::runtime_error(message);
}

struct Encoder {
	vorbis_info info{};
	vorbis_comment comment{};
	vorbis_dsp_state dsp{};
	vorbis_block block{};
	ogg_stream_state stream{};
	bool info_ready = false;
	bool comment_ready = false;
	bool dsp_ready = false;
	bool block_ready = false;
	bool stream_ready = false;

	~Encoder()
	{
		if (stream_ready)
			ogg_stream_clear(&stream);
		if (block_ready)
			vorbis_block_clear(&block);
		if (dsp_ready)
			vorbis_dsp_clear(&dsp);
		if (comment_ready)
			vorbis_comment_clear(&comment);
		if (info_ready)
			vorbis_info_clear(&info);
	}
};

void append_page(std::vector<unsigned char> &out, const ogg_page &page)
{
	out.insert(out.end(), page.header, page.header + page.header_len);
	out.insert(out.end(), page.body, page.body + page.body_len);
}

void flush_pages(Encoder &enc, std::vector<unsigned char> &out, bool flush)
{
	ogg_page page{};
	auto write = flush ? ogg_stream_flush : ogg_stream_pageout;
	while (write(&enc.stream, &page))
		append_page(out, page);
}

std::vector<unsigned char> make_ogg_tone(int rate, int channels, int frames)
{
	Encoder enc;
	vorbis_info_init(&enc.info);
	enc.info_ready = true;
	require(vorbis_encode_init_vbr(&enc.info, channels, rate, 0.35f) == 0,
		"vorbis_encode_init_vbr failed");
	vorbis_comment_init(&enc.comment);
	enc.comment_ready = true;
	vorbis_comment_add_tag(&enc.comment, const_cast<char *>("ENCODER"),
		const_cast<char *>("MundoCraft audio host smoke test"));
	require(vorbis_analysis_init(&enc.dsp, &enc.info) == 0,
		"vorbis_analysis_init failed");
	enc.dsp_ready = true;
	require(vorbis_block_init(&enc.dsp, &enc.block) == 0,
		"vorbis_block_init failed");
	enc.block_ready = true;
	require(ogg_stream_init(&enc.stream, 0x4d433032) == 0,
		"ogg_stream_init failed");
	enc.stream_ready = true;

	std::vector<unsigned char> encoded;
	ogg_packet header{}, header_comments{}, header_codebooks{};
	require(vorbis_analysis_headerout(&enc.dsp, &enc.comment, &header,
		&header_comments, &header_codebooks) == 0, "Vorbis header generation failed");
	for (ogg_packet *packet : {&header, &header_comments, &header_codebooks}) {
		require(ogg_stream_packetin(&enc.stream, packet) == 0,
			"Ogg header packet insertion failed");
		flush_pages(enc, encoded, true);
	}

	auto drain_encoder = [&]() {
		while (vorbis_analysis_blockout(&enc.dsp, &enc.block) == 1) {
			require(vorbis_analysis(&enc.block, nullptr) == 0,
				"vorbis_analysis failed");
			require(vorbis_bitrate_addblock(&enc.block) == 0,
				"vorbis_bitrate_addblock failed");
			ogg_packet packet{};
			while (vorbis_bitrate_flushpacket(&enc.dsp, &packet)) {
				require(ogg_stream_packetin(&enc.stream, &packet) == 0,
					"Ogg audio packet insertion failed");
				flush_pages(enc, encoded, false);
			}
		}
	};

	for (int first = 0; first < frames; first += 1024) {
		const int count = std::min(1024, frames - first);
		float **pcm = vorbis_analysis_buffer(&enc.dsp, count);
		require(pcm != nullptr, "vorbis_analysis_buffer failed");
		for (int i = 0; i < count; ++i) {
			const double phase = 2.0 * 3.14159265358979323846 * 440.0 * (first + i) / rate;
			const float sample = static_cast<float>(0.35 * std::sin(phase));
			for (int channel = 0; channel < channels; ++channel)
				pcm[channel][i] = sample;
		}
		require(vorbis_analysis_wrote(&enc.dsp, count) == 0,
			"vorbis_analysis_wrote failed");
		drain_encoder();
	}
	require(vorbis_analysis_wrote(&enc.dsp, 0) == 0,
		"Vorbis end-of-stream signal failed");
	drain_encoder();
	flush_pages(enc, encoded, true);
	require(encoded.size() > 100, "generated Ogg stream is unexpectedly small");
	return encoded;
}

struct MemoryInput {
	const std::vector<unsigned char> *bytes;
	size_t offset = 0;
};

size_t read_memory(void *destination, size_t size, size_t count, void *opaque)
{
	if (!size)
		return 0;
	auto &input = *static_cast<MemoryInput *>(opaque);
	const size_t available = input.bytes->size() - input.offset;
	const size_t items = std::min(count, available / size);
	const size_t bytes = items * size;
	if (bytes)
		std::memcpy(destination, input.bytes->data() + input.offset, bytes);
	input.offset += bytes;
	return items;
}

int seek_memory(void *opaque, ogg_int64_t offset, int whence)
{
	auto &input = *static_cast<MemoryInput *>(opaque);
	ogg_int64_t base = 0;
	if (whence == SEEK_CUR)
		base = static_cast<ogg_int64_t>(input.offset);
	else if (whence == SEEK_END)
		base = static_cast<ogg_int64_t>(input.bytes->size());
	else if (whence != SEEK_SET)
		return -1;
	const ogg_int64_t target = base + offset;
	if (target < 0 || static_cast<uint64_t>(target) > input.bytes->size())
		return -1;
	input.offset = static_cast<size_t>(target);
	return 0;
}

int close_memory(void *) { return 0; }

long tell_memory(void *opaque)
{
	const size_t offset = static_cast<MemoryInput *>(opaque)->offset;
	if (offset > static_cast<size_t>(std::numeric_limits<long>::max()))
		return -1;
	return static_cast<long>(offset);
}

std::vector<int16_t> decode_ogg(const std::vector<unsigned char> &encoded,
		int expected_rate, int expected_channels)
{
	MemoryInput input{&encoded};
	ov_callbacks callbacks{read_memory, seek_memory, close_memory, tell_memory};
	OggVorbis_File file{};
	require(ov_open_callbacks(&input, &file, nullptr, 0, callbacks) == 0,
		"ov_open_callbacks rejected generated Ogg stream");
	vorbis_info *info = ov_info(&file, -1);
	require(info && info->rate == expected_rate && info->channels == expected_channels,
		"decoded stream format does not match encoder input");
	std::vector<unsigned char> bytes;
	char chunk[4096];
	int bitstream = 0;
	for (;;) {
		const long amount = ov_read(&file, chunk, sizeof(chunk), 0, 2, 1, &bitstream);
		if (amount == 0)
			break;
		require(amount > 0, "ov_read reported a Vorbis decode error");
		bytes.insert(bytes.end(), chunk, chunk + amount);
	}
	ov_clear(&file);
	require(bytes.size() >= 4096 && bytes.size() % sizeof(int16_t) == 0,
		"Vorbis decoder returned too few or misaligned PCM bytes");
	std::vector<int16_t> pcm(bytes.size() / sizeof(int16_t));
	std::memcpy(pcm.data(), bytes.data(), bytes.size());
	require(std::any_of(pcm.begin(), pcm.end(), [](int16_t value) { return value != 0; }),
		"decoded PCM is silent");
	return pcm;
}

using LoopbackOpen = ALCdevice *(ALC_APIENTRY *)(const ALCchar *);
using RenderSamples = void (ALC_APIENTRY *)(ALCdevice *, ALCvoid *, ALCsizei);
using RenderFormatSupported = ALCboolean (ALC_APIENTRY *)(ALCdevice *, ALCsizei,
		ALCenum, ALCenum);

void render_and_reopen(const std::vector<int16_t> &pcm, int rate)
{
	require(alcIsExtensionPresent(nullptr, "ALC_SOFT_loopback") == ALC_TRUE,
		"OpenAL Soft loopback extension is unavailable");
	auto open_loopback = reinterpret_cast<LoopbackOpen>(
		alcGetProcAddress(nullptr, "alcLoopbackOpenDeviceSOFT"));
	auto render_samples = reinterpret_cast<RenderSamples>(
		alcGetProcAddress(nullptr, "alcRenderSamplesSOFT"));
	auto format_supported = reinterpret_cast<RenderFormatSupported>(
		alcGetProcAddress(nullptr, "alcIsRenderFormatSupportedSOFT"));
	require(open_loopback && render_samples && format_supported,
		"OpenAL Soft loopback entry points are missing");

	for (int pass = 0; pass < 2; ++pass) {
		ALCdevice *device = open_loopback(nullptr);
		require(device != nullptr, "alcLoopbackOpenDeviceSOFT failed");
		if (!format_supported(device, rate, ALC_STEREO_SOFT, ALC_SHORT_SOFT)) {
			alcCloseDevice(device);
			throw std::runtime_error("OpenAL Soft rejected stereo signed-16 loopback format");
		}
		const ALCint attributes[] = {
			ALC_FREQUENCY, rate,
			ALC_FORMAT_CHANNELS_SOFT, ALC_STEREO_SOFT,
			ALC_FORMAT_TYPE_SOFT, ALC_SHORT_SOFT,
			0
		};
		ALCcontext *context = alcCreateContext(device, attributes);
		if (!context) {
			alcCloseDevice(device);
			throw std::runtime_error("alcCreateContext failed for loopback device");
		}
		require(alcMakeContextCurrent(context) == ALC_TRUE,
			"alcMakeContextCurrent failed");

		ALuint buffer = 0;
		ALuint source = 0;
		alGenBuffers(1, &buffer);
		require(alGetError() == AL_NO_ERROR, "alGenBuffers failed");
		alBufferData(buffer, AL_FORMAT_STEREO16, pcm.data(),
			static_cast<ALsizei>(pcm.size() * sizeof(int16_t)), rate);
		require(alGetError() == AL_NO_ERROR, "alBufferData failed");
		alGenSources(1, &source);
		require(alGetError() == AL_NO_ERROR, "alGenSources failed");
		alSourcei(source, AL_BUFFER, static_cast<ALint>(buffer));
		alSourcePlay(source);
		require(alGetError() == AL_NO_ERROR, "alSourcePlay failed");

		constexpr ALCsizei render_frames = 4096;
		std::vector<int16_t> output(static_cast<size_t>(render_frames) * 2);
		render_samples(device, output.data(), render_frames);
		const auto max_sample = *std::max_element(output.begin(), output.end(),
			[](int16_t a, int16_t b) { return std::abs(static_cast<int>(a)) <
				std::abs(static_cast<int>(b)); });
		require(std::abs(static_cast<int>(max_sample)) > 100,
			"OpenAL loopback rendered silence or an implausibly quiet signal");

		alSourceStop(source);
		require(alGetError() == AL_NO_ERROR, "alSourceStop failed");
		alDeleteSources(1, &source);
		alDeleteBuffers(1, &buffer);
		require(alGetError() == AL_NO_ERROR, "OpenAL source/buffer cleanup failed");		
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(context);
		require(alcCloseDevice(device) == ALC_TRUE,
			"alcCloseDevice failed during loopback cleanup");
		std::cout << "loopback pass " << (pass + 1) << ": max PCM="
			<< std::abs(static_cast<int>(max_sample)) << '\n';
	}
}

} // namespace

int main()
{
	try {
		constexpr int sample_rate = 44100;
		constexpr int channels = 2;
		const auto encoded = make_ogg_tone(sample_rate, channels, sample_rate / 2);
		const auto pcm = decode_ogg(encoded, sample_rate, channels);
		render_and_reopen(pcm, sample_rate);
		std::cout << "PASS: generated Ogg/Vorbis encoded " << encoded.size()
			<< " bytes; decoded " << (pcm.size() / channels)
			<< " stereo frames; rendered nonzero audio and reopened loopback context.\n";
		return 0;
	} catch (const std::exception &e) {
		std::cerr << "FAIL: " << e.what() << '\n';
		return 1;
	}
}
