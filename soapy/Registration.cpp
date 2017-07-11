
#include <mlsdr/mlsdr.h>
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Types.hpp>

#include "SoapyMLSDR.hpp"

using namespace std;

static SoapySDR::KwargsList findIConnection(const SoapySDR::Kwargs &args)
{
	SoapySDR::KwargsList results;

	struct mlsdr_desc *desc = mlsdr_enumerate();

	while (desc != NULL) {
		if (args.count("serno") == 0 || (args.at("serno") == desc->serno)) {
			SoapySDR::Kwargs devargs;
			string serno = desc->serno;
			devargs["serno"] = serno;
			devargs["label"] = "mlsdr :: " + devargs["serno"];
			results.push_back(devargs);
		}
		desc = desc->next;
	}

	return results;
}

static SoapySDR::Device *makeIConnection(const SoapySDR::Kwargs &args)
{
	return new SoapyMLSDR(args);
}

static SoapySDR::Registry registerIConnection("mlsdr", &findIConnection,
											  &makeIConnection,
											  SOAPY_SDR_ABI_VERSION);
