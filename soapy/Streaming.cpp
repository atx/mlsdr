
#include <SoapySDR/Logger.h>
#include <SoapySDR/Formats.hpp>

#include <mlsdr/mlsdr.h>
#include <mlsdr/r820t.h>

#include "SoapyMLSDR.hpp"

#define BYTES_PER_SAMPLE	2

std::vector<std::string> SoapyMLSDR::getStreamFormats(const int direction, const size_t channel) const
{
	std::vector<std::string> formats;

	formats.push_back(SOAPY_SDR_S16);
	// We need this for gqrx
	formats.push_back(SOAPY_SDR_CF32);

	return formats;
}

std::string SoapyMLSDR::getNativeStreamFormat(const int direction,
										const size_t channel, double &fullScale) const
{
	fullScale = 2048;
	return SOAPY_SDR_S16;
	//fullScale = 1.0;
	//return SOAPY_SDR_CF32;
}

SoapySDR::ArgInfoList SoapyMLSDR::getStreamArgsInfo(
									const int direction,
									const size_t channel) const
{
	if (direction != SOAPY_SDR_RX) {
		throw std::runtime_error("MLSDR is RX only, use SOAPY_SDR_RX");
	}

	SoapySDR::ArgInfoList streamArgs;

	SoapySDR::ArgInfo bufflenArg;
	bufflenArg.key = "bufflen";
	bufflenArg.value = "16384";
	bufflenArg.name = "Buffer Size";
	bufflenArg.description = "Number of bytes per buffer, multiples of 512 only.";
	bufflenArg.units = "bytes";
	bufflenArg.type = SoapySDR::ArgInfo::INT;

	streamArgs.push_back(bufflenArg);

	SoapySDR::ArgInfo buffersArg;
	buffersArg.key = "buffers";
	buffersArg.value = "15";
	buffersArg.name = "Buffer Count";
	buffersArg.description = "Number of buffers per read.";
	buffersArg.units = "buffers";
	buffersArg.type = SoapySDR::ArgInfo::INT;

	streamArgs.push_back(buffersArg);

	return streamArgs;
}

/*******************************************************************
 * Stream API
 ******************************************************************/

SoapySDR::Stream *SoapyMLSDR::setupStream(
									const int direction,
									const std::string &format,
									const std::vector<size_t> &channels,
									const SoapySDR::Kwargs &args)
{
	if (direction != SOAPY_SDR_RX) {
		throw std::runtime_error("MLSDR is RX only, use SOAPY_SDR_RX");
	}

	if (channels.size() > 1 || (channels.size() > 0 && channels.at(0) != 0)) {
		throw std::runtime_error("setupStream invalid channel selection");
	}

	if (format != SOAPY_SDR_S16 && format != SOAPY_SDR_CF32) {
		throw std::runtime_error("setupStream invalid format '" + format + "'");
	}

	this->streamFormat = format;
	this->streamTmp = new int16_t[this->getStreamMTU((SoapySDR::Stream *)this)];

	return (SoapySDR::Stream *)this;
}

void SoapyMLSDR::closeStream(SoapySDR::Stream *stream)
{
	delete[] this->streamTmp;
}

size_t SoapyMLSDR::getStreamMTU(SoapySDR::Stream *stream) const
{
	return 1000000;
}

int SoapyMLSDR::activateStream(
		SoapySDR::Stream *stream,
		const int flags,
		const long long timeNs,
		const size_t numElems)
{
	if (flags != 0) {
		return SOAPY_SDR_NOT_SUPPORTED;
	}

	mlsdr_adc_enable(mlsdr);

	return 0;
}

int SoapyMLSDR::deactivateStream(
		SoapySDR::Stream *stream,
		const int flags,
		const long long timeNs)
{
	if (flags != 0) {
		return SOAPY_SDR_NOT_SUPPORTED;
	}

	mlsdr_adc_disable(this->mlsdr);

	return 0;
}

int SoapyMLSDR::readStream(
		SoapySDR::Stream *stream,
		void *const *buffs,
		const size_t numElems,
		int &flags,
		long long &timeNs,
		const long timeoutUs)
{
	void *buff0 = buffs[0];
	long timeoutMs = timeoutUs / 1000;
	if (timeoutMs == 0) {
		timeoutMs = 1;
	}

	if (this->streamFormat == SOAPY_SDR_S16) {
		// We have easy job in this case
		int16_t *samples = (int16_t *)buff0;
		return mlsdr_read(this->mlsdr, samples, numElems, timeoutMs);
	}

	if (numElems > this->getStreamMTU(stream)) {
		SoapySDR_logf(SOAPY_SDR_ERROR, "readStream cannot read more than MTU bytes at once");
	}

	if (this->streamFormat == SOAPY_SDR_CF32) {
		std::complex<float> *samples = (std::complex<float> *)buff0;
		if (this->iqconv) {
			ssize_t ret = mlsdr_read_iq(this->mlsdr, samples, numElems, timeoutMs);
			return ret;
		} else {
			// Pad with zeroes, this produces negative frequencies
			int ret = mlsdr_read(this->mlsdr, this->streamTmp, numElems, timeoutMs);
			if (ret < 0) {
				return ret;
			}
			for (int i = 0; i < ret; i++) {
				samples[i] = (float)this->streamTmp[i] / 2048.0;
			}

			return ret;
		}
	}

	return 0;
}
