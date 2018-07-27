/*
 * RegionSampling.h
 *
 *  Created on: 18.03.2015
 *      Author: mheinen
 */

#ifndef REGIONSAMPLING_H_
#define REGIONSAMPLING_H_

#include "utils/ObserverBase.h"
#include "utils/Region.h"
#include "molecules/MoleculeForwardDeclaration.h"
#include "plugins/PluginBase.h"

#include <vector>
#include <array>
#include <cstdint>

enum RegionSamplingDimensions
{
	RS_DIMENSION_X = 0,
	RS_DIMENSION_Y = 1,
	RS_DIMENSION_Z = 2,
};

enum RegionSamplingFileTypes : uint8_t
{
	RSFT_ASCII = 1,
	RSFT_BINARY = 2
};

struct ComponentSpecificParamsVDF
{
	bool bSamplingEnabled;
	double dVeloMax;
	double dVelocityClassWidth;
	double dInvVelocityClassWidth;
	uint32_t numVelocityClasses;
	uint32_t numVals;
	uint32_t nOffsetDataStructure;
	std::vector<double> dDiscreteVelocityValues;
};

class XMLfileUnits;
class Domain;
class DomainDecompBase;
class RegionSampling;

class SampleRegion : public CuboidRegionObs
{
public:
	SampleRegion(RegionSampling* parent, double dLowerCorner[3], double dUpperCorner[3] );
	virtual ~SampleRegion();

	void readXML(XMLfileUnits& xmlconfig);

	// set parameters
	void setParamProfiles( unsigned long initSamplingProfiles, unsigned long writeFrequencyProfiles, unsigned long stopSamplingProfiles)
	{
		_initSamplingProfiles   = initSamplingProfiles;
		_writeFrequencyProfiles = writeFrequencyProfiles;
		_stopSamplingProfiles   = stopSamplingProfiles;
		_SamplingEnabledProfiles = true;
		_dInvertNumSamplesProfiles = 1. / ( (double)(_writeFrequencyProfiles) );  // needed for e.g. density profiles
	}
	void setParamProfiles()
	{
		_SamplingEnabledProfiles = true;
		_dInvertNumSamplesProfiles = 1. / ( (double)(_writeFrequencyProfiles) );  // needed for e.g. density profiles
	}

	// subdivision
	void setSubdivisionProfiles(const unsigned int& nNumSlabs) {_nNumBinsProfiles = nNumSlabs; _nSubdivisionOptProfiles = SDOPT_BY_NUM_SLABS;}
	void setSubdivisionProfiles(const double& dSlabWidth) {_dBinWidthProfilesInit = dSlabWidth; _nSubdivisionOptProfiles = SDOPT_BY_SLAB_WIDTH;}
	void setSubdivisionVDF(const unsigned int& nNumSlabs) {_numBinsVDF = nNumSlabs; _nSubdivisionOptVDF = SDOPT_BY_NUM_SLABS;}
	void setSubdivisionVDF(const double& dSlabWidth) {_dBinWidthVDFInit = dSlabWidth; _nSubdivisionOptVDF = SDOPT_BY_SLAB_WIDTH;}
	void prepareSubdivisionProfiles();  // need to be called before data structure allocation
	void prepareSubdivisionVDF();  		// need to be called before data structure allocation
	void prepareSubdivisionFieldYR();  	// need to be called before data structure allocation


	// init sampling
	void initSamplingProfiles(int nDimension);
	void initSamplingVDF(int nDimension);
	void initSamplingFieldYR(int nDimension);
	void doDiscretisationProfiles(int nDimension);
	void doDiscretisationVDF(int nDimension);
	void doDiscretisationFieldYR(int nDimension);

	// molecule container loop methods
	void sampleProfiles(Molecule* molecule, int nDimension);
	void sampleVDF(Molecule* molecule, int nDimension);
	void sampleFieldYR(Molecule* molecule);

	// calc global values
	void calcGlobalValuesProfiles(DomainDecompBase* domainDecomp, Domain* domain);
	void calcGlobalValuesVDF();
	void calcGlobalValuesFieldYR(DomainDecompBase* domainDecomp, Domain* domain);

	// output
	void writeDataProfiles(DomainDecompBase* domainDecomp, unsigned long simstep, Domain* domain);
	void writeDataVDF(DomainDecompBase* domainDecomp, unsigned long simstep);
	void writeDataFieldYR(DomainDecompBase* domainDecomp, unsigned long simstep, Domain* domain);

	void updateSlabParameters();

private:
	// reset local values
	void resetLocalValuesProfiles();
	void resetOutputDataProfiles();
	void resetLocalValuesVDF();
	void resetLocalValuesFieldYR();

	void initComponentSpecificParamsVDF();
	void showComponentSpecificParamsVDF();

	static void get_v(double* q, Molecule* mol);
	static void get_F(double* q, Molecule* mol);
	static void get_v2(double& q, Molecule* mol);
	static void get_F2(double& q, Molecule* mol);
	void(*_fptr)(double*, Molecule*);
	void(*_f2ptr)(double&, Molecule*);

private:
	// instances / ID
	static unsigned short _nStaticID;

	uint32_t _numComponents;

	// ******************
	// sampling variables
	// ******************

	// --- temperature / density profiles, SamplingZone ---

	// control
	bool _bDiscretisationDoneProfiles;
	bool _SamplingEnabledProfiles;
	int _nSubdivisionOptProfiles;

	// parameters
	unsigned long _initSamplingProfiles;
	unsigned long _writeFrequencyProfiles;
	unsigned long _stopSamplingProfiles;
	unsigned int _nNumBinsProfiles;

	double  _dBinWidthProfiles;
	double  _dBinWidthProfilesInit;
	double  _dBinVolumeProfiles;
	double* _dBinMidpointsProfiles;

	// offsets
	// TODO: Use only 1 offset array: for scalar one can take the offsets of dimension x, right???
	unsigned long**  _nOffsetScalar;  //                  [direction all|+|-][component]
	unsigned long*** _nOffsetVector;  // [dimension x|y|z][direction all|+|-][component]

	unsigned long _nNumValsScalar;
	unsigned long _nNumValsVector;

	std::vector<double> _vecMass;
	double _dInvertNumSamplesProfiles;
	double _dInvertBinVolumeProfiles;
	double _dInvertBinVolSamplesProfiles;

	// Scalar quantities
	// [direction all|+|-][component][position]
	unsigned long* _nNumMoleculesLocal;
	unsigned long* _nNumMoleculesGlobal;
	unsigned long* _nRotDOFLocal;
	unsigned long* _nRotDOFGlobal;
	double* _d2EkinRotLocal;
	double* _d2EkinRotGlobal;

	// output profiles
	double* _dDensity;
	double* _d2EkinTotal;
	double* _d2EkinTrans;
	double* _d2EkinDrift;
	double* _d2EkinRot;
	double* _d2EkinT;
	double* _dTemperature;
	double* _dTemperatureTrans;
	double* _dTemperatureRot;

	// Vector quantities
	// [dimension x|y|z][direction all|+|-][component][position]
	double* _dVelocityLocal;
	double* _dVelocityGlobal;
	double* _dSquaredVelocityLocal;
	double* _dSquaredVelocityGlobal;
	double* _dForceLocal;
	double* _dForceGlobal;

	// output profiles
	double* _dForce;
	double* _dDriftVelocity;
	double* _d2EkinTransComp;
	double* _d2EkinDriftComp;
	double* _dTemperatureComp;

	// --- VDF ---

	// parameters
	unsigned long _initSamplingVDF;
	unsigned long _writeFrequencyVDF;
	unsigned long _stopSamplingVDF;

	// control
	bool _bDiscretisationDoneVDF;
	bool _SamplingEnabledVDF;
	int _nSubdivisionOptVDF;

	double _dBinWidthVDF;
	double _dInvBinWidthVDF;
	double _dBinWidthVDFInit;
	std::vector<double> _dBinMidpointsVDF;

	// component specific parameters
	std::vector<ComponentSpecificParamsVDF> _vecComponentSpecificParamsVDF;

	// data structures
	uint32_t _numBinsVDF;
	uint32_t _numValsVDF;

	// local
	uint64_t* _VDF_pjy_abs_local;
	uint64_t* _VDF_pjy_pvx_local;
	uint64_t* _VDF_pjy_pvy_local;
	uint64_t* _VDF_pjy_pvz_local;
	uint64_t* _VDF_pjy_nvx_local;
	uint64_t* _VDF_pjy_nvz_local;

	uint64_t* _VDF_njy_abs_local;
	uint64_t* _VDF_njy_pvx_local;
	uint64_t* _VDF_njy_pvz_local;
	uint64_t* _VDF_njy_nvx_local;
	uint64_t* _VDF_njy_nvy_local;
	uint64_t* _VDF_njy_nvz_local;

	// global
	uint64_t* _VDF_pjy_abs_global;
	uint64_t* _VDF_pjy_pvx_global;
	uint64_t* _VDF_pjy_pvy_global;
	uint64_t* _VDF_pjy_pvz_global;
	uint64_t* _VDF_pjy_nvx_global;
	uint64_t* _VDF_pjy_nvz_global;

	uint64_t* _VDF_njy_abs_global;
	uint64_t* _VDF_njy_pvx_global;
	uint64_t* _VDF_njy_pvz_global;
	uint64_t* _VDF_njy_nvx_global;
	uint64_t* _VDF_njy_nvy_global;
	uint64_t* _VDF_njy_nvz_global;

	std::array<std::array<uint64_t*,4>,3> _dataPtrs;
	std::string _fnamePrefixVDF;

	// --- fieldYR ---

	// control
	bool _bDiscretisationDoneFieldYR;
	bool _SamplingEnabledFieldYR;
	int _nSubdivisionOptFieldYR_Y;
	int _nSubdivisionOptFieldYR_R;

	// parameters
	uint64_t _initSamplingFieldYR;
	uint64_t _writeFrequencyFieldYR;
	uint64_t _stopSamplingFieldYR;
	uint32_t _nNumBinsFieldYR;
	uint32_t _nNumShellsFieldYR;
	double  _dBinWidthInitFieldYR;
	double  _dShellWidthInitFieldYR;

	double  _dBinWidthFieldYR;
	double  _dShellWidthFieldYR;
	double  _dShellWidthSquaredFieldYR;
	double* _dBinMidpointsFieldYR;
	double* _dShellMidpointsFieldYR;
	double* _dShellVolumesFieldYR;
	double* _dInvShellVolumesFieldYR;
	// equal shell volumes
	double _dShellVolumeFieldYR;
	double _dInvShellVolumeFieldYR;

	// offsets
	uint64_t**** _nOffsetFieldYR;  // [dimension x|y|z][component][section][positionR], dimension = 0 for scalar values, section: 0: all, 1: upper, 2: lower section
	uint64_t _nNumValsFieldYR;

	// Scalar quantities
	// [component][section][positionR][positionY]
	uint64_t* _nNumMoleculesFieldYRLocal;
	uint64_t* _nNumMoleculesFieldYRGlobal;

	// output profiles
	double* _dDensityFieldYR;

	// output file
	std::string _strFilePrefixFieldYR;
	uint8_t _nFileTypeFieldYR;
};

class XMLfileUnits;
class RegionSampling : public ControlInstance, public PluginBase
{
public:
	RegionSampling();
	virtual ~RegionSampling();

	std::string getShortName() override {return "ReS";}

	void readXML(XMLfileUnits& xmlconfig) override;

	void init(ParticleContainer *particleContainer,
			  DomainDecompBase *domainDecomp, Domain *domain) override;

	void afterForces(
			ParticleContainer* particleContainer, DomainDecompBase* domainDecomp,
			unsigned long simstep
	) override {}

	void endStep(
			ParticleContainer *particleContainer,
			DomainDecompBase *domainDecomp, Domain *domain,
			unsigned long simstep) override;

	void finish(ParticleContainer *particleContainer,
				DomainDecompBase *domainDecomp, Domain *domain) override {}

	std::string getPluginName() override {return std::string("RegionSampling");}
	static PluginBase* createInstance() {return new RegionSampling();}

protected:
	void addRegion(SampleRegion* region);
	int getNumRegions() {return _vecSampleRegions.size();}
	SampleRegion* getSampleRegion(unsigned short nRegionID) {return _vecSampleRegions.at(nRegionID-1); }  // vector index starts with 0, region index with 1

	// sample profiles and vdf
	void doSampling(Molecule* mol, DomainDecompBase* domainDecomp, unsigned long simstep);
	// write out profiles and vdf
	void writeData(DomainDecompBase* domainDecomp, unsigned long simstep);
	void prepareRegionSubdivisions();  // need to be called before allocating the data structures

private:
	std::vector<SampleRegion*> _vecSampleRegions;

	unsigned long _initSamplingProfiles;
	unsigned long _writeFrequencyProfiles;
	unsigned long _initSamplingVDF;
	unsigned long _writeFrequencyVDF;
};


//class RegionSamplingPlugin : public RegionSampling, public PluginBase
//{
//public:
//	RegionSamplingPlugin() {}
//	~RegionSamplingPlugin() {}
//
//	void readXML(XMLfileUnits& xmlconfig) override;
//
//	void init(ParticleContainer *particleContainer,
//			  DomainDecompBase *domainDecomp, Domain *domain) override;
//
//	void afterForces(
//			ParticleContainer* particleContainer, DomainDecompBase* domainDecomp,
//			unsigned long simstep
//	) override;
//
//	void endStep(
//			ParticleContainer *particleContainer,
//			DomainDecompBase *domainDecomp, Domain *domain,
//			unsigned long simstep) override {}
//
//	void finish(ParticleContainer *particleContainer,
//				DomainDecompBase *domainDecomp, Domain *domain) override {}
//
//	std::string getPluginName() override {return std::string("RegionSamplingPlugin");}
//	static PluginBase* createInstance() {return new RegionSamplingPlugin();}
//};

#endif /* REGIONSAMPLING_H_ */
