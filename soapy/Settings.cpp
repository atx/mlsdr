
#include <SoapySDR/Logger.h>
#include <SoapySDR/Formats.h>

#include <mlsdr/mlsdr.h>
#include <mlsdr/r820t.h>

#include "SoapyMLSDR.hpp"

SoapyMLSDR::SoapyMLSDR(const SoapySDR::Kwargs &args)
	:
	iqconv(true)
{
	struct mlsdr_connect_cfg cfg = MLSDR_DEFAULT_CFG;
	// TODO: Probably tunnel the logctx to SoapySDR
	if (args.count("serno") != 0) {
		cfg.serno = args.at("serno").c_str();
	}
	if (args.count("iqconv") != 0) {
		this->iqconv = args.at("iqconv") == "1";
	}

	cfg.sample_buf_size = 100000000;

	this->mlsdr = mlsdr_connect(cfg);
	if (this->mlsdr == NULL) {
		throw std::runtime_error("mlsdr_connect failed!");
	}
}

SoapyMLSDR::~SoapyMLSDR(void)
{
	mlsdr_destroy(mlsdr);
}

/*******************************************************************
 * Identification API
 ******************************************************************/

std::string SoapyMLSDR::getDriverKey(void) const
{
	return "MLSDR";
}

std::string SoapyMLSDR::getHardwareKey(void) const
{
	// TODO: Add serial string (first needs to be added to libmlsdr)
	return "MLSDR";
}

SoapySDR::Kwargs SoapyMLSDR::getHardwareInfo(void) const
{
	SoapySDR::Kwargs args;

	args["origin"] = "https://github.com/atalax/mlaticka";

	return args;
}

size_t SoapyMLSDR::getNumChannels(const int dir) const
{
	return (dir == SOAPY_SDR_RX) ? 1 : 0;
}

std::vector<std::string> SoapyMLSDR::listAntennas(const int direction,
												  const size_t channel) const
{
	std::vector<std::string> antennas;
	antennas.push_back("RX");
	return antennas;
}

void SoapyMLSDR::setAntenna(const int direction, const size_t channel,
							const std::string &name)
{
	if (direction != SOAPY_SDR_RX) {
		throw std::runtime_error("setAntenna failed: MLSDR only supports RX");
	}
}

std::string SoapyMLSDR::getAntenna(const int direction, const size_t channel) const
{
	return "RX";
}

/*******************************************************************
 * Frontend corrections API
 ******************************************************************/

bool SoapyMLSDR::hasDCOffsetMode(const int direction, const size_t channel) const
{
	return false;
}

/*******************************************************************
 * Gain API
 ******************************************************************/

std::vector<std::string> SoapyMLSDR::listGains(const int direction,
											   const size_t channel) const
{
	std::vector<std::string> results;

	results.push_back("LNA");
	results.push_back("MIXER");
	results.push_back("VGA");

	return results;
}

bool SoapyMLSDR::hasGainMode(const int direction, const size_t channel) const
{
	return true;
}

void SoapyMLSDR::setGainMode(const int direction, const size_t channel,
							 const bool automatic)
{
	if (automatic) {
		r820t_autogain(this->mlsdr->r820t);
	}
}

bool SoapyMLSDR::getGainMode(const int direction, const size_t channel) const
{
	return SoapySDR::Device::getGainMode(direction, channel);
}

static inline enum r820t_gain_stage nameToStage(const std::string &name)
{
	if (name == "LNA") {
		return R820T_GAIN_LNA;
	}
	if (name == "MIXER") {
		return R820T_GAIN_MIXER;
	}
	if (name == "VGA") {
		return R820T_GAIN_VGA;
	}

	throw std::runtime_error("Invalid gain stage name");
}

void SoapyMLSDR::setGain(const int direction, const size_t channel,
						 const std::string &name, const double value)
{
	int gain = (int)(value * 10);
	r820t_set_gain_stage(this->mlsdr->r820t, nameToStage(name), gain, NULL);
}

SoapySDR::Range SoapyMLSDR::getGainRange(const int direction, const size_t channel,
										 const std::string &name) const
{
	const int *values;
	size_t len = r820t_get_gain_stage_steps(nameToStage(name), &values);

	int min = values[0];
	int max = values[0];
	for (size_t i = 1; i < len; i++) {
		if (values[i] < min) {
			min = values[i];
		}
		if (values[i] > max) {
			max = values[i];
		}
	}

	return SoapySDR::Range((double)min / 10, (double)max / 10);
}

void SoapyMLSDR::setFrequency(const int direction, const size_t channel,
							  const std::string &name, const double frequency,
							  const SoapySDR::Kwargs &args)
{
	this->frequency = (int)frequency;
	r820t_tune(this->mlsdr->r820t, (int)frequency);
}

double SoapyMLSDR::getFrequency(const int direction, const size_t channel,
								const std::string &name) const
{
	return this->frequency;
}

std::vector<std::string> SoapyMLSDR::listFrequencies(
											const int direction,
											const size_t channel) const
{
	std::vector<std::string> names;
	names.push_back("RF");
	return names;
}

SoapySDR::RangeList SoapyMLSDR::getFrequencyRange(
				const int direction,
				const size_t channel,
				const std::string &name) const
{
	SoapySDR::RangeList results;
	results.push_back(SoapySDR::Range(24000000, 1764000000));
	return results;
}

/*******************************************************************
 * Sample Rate API
 ******************************************************************/

void SoapyMLSDR::setSampleRate(const int direction, const size_t channel, const double rate)
{
	return;
}

double SoapyMLSDR::getSampleRate(const int direction, const size_t channel) const
{
	if (this->streamFormat == SOAPY_SDR_CF32 && this->iqconv) {
		return 7500000;
	}
	return 15000000;
}

std::vector<double> SoapyMLSDR::listSampleRates(const int direction, const size_t channel) const
{
	std::vector<double> results;
	results.push_back(15000000);
	results.push_back(7500000);
	return results;
}
