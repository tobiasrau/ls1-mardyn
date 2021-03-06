/*
 * TemperatureControl.h
 *
 *  Created on: 27.05.2015
 *      Author: mheinen
 */

#ifndef TEMPERATURECONTROL_H_
#define TEMPERATURECONTROL_H_

#include <vector>
#include <string>

#include "molecules/Molecule.h"
#include "ThermostatVariables.h"
#include "utils/Random.h"
#include "integrators/Integrator.h"

class XMLfileUnits;
class DomainDecompBase;
class ParticleContainer;
class Accumulator;
class ControlRegionT
{
public:
	ControlRegionT();
	~ControlRegionT();

	void readXML(XMLfileUnits& xmlconfig);

	unsigned int GetID(){return _nID;}
	void VelocityScalingInit(XMLfileUnits &xmlconfig, std::string strDirections);

	double* GetLowerCorner() {return _dLowerCorner;}
	double* GetUpperCorner() {return _dUpperCorner;}
	void SetLowerCorner(unsigned short nDim, double dVal) {_dLowerCorner[nDim] = dVal;}
	void SetUpperCorner(unsigned short nDim, double dVal) {_dUpperCorner[nDim] = dVal;}
	double GetWidth(unsigned short nDim) {return _dUpperCorner[nDim] - _dLowerCorner[nDim];}
	void GetRange(unsigned short nDim, double& dRangeBegin, double& dRangeEnd) {dRangeBegin = _dLowerCorner[nDim]; dRangeEnd = _dUpperCorner[nDim];}
	void CalcGlobalValues(DomainDecompBase* domainDecomp);
	void MeasureKineticEnergy(Molecule* mol, DomainDecompBase* domainDecomp);

	void ControlTemperature(Molecule* mol);

	void ResetLocalValues();

	// beta log file
	void InitBetaLogfile();
	void WriteBetaLogfile(unsigned long simstep);

	enum LocalControlMethod {
		VelocityScaling,
		Andersen,
	};
	LocalControlMethod _localMethod;

private:
	// create accumulator object dependent on which translatoric directions should be thermostated (xyz)
	Accumulator* CreateAccumulatorInstance(std::string strTransDirections);

	// instances / ID
	static unsigned short _nStaticID;
	unsigned short _nID;

	double _dLowerCorner[3];
	double _dUpperCorner[3];

	unsigned int _nNumSlabs;
	double _dSlabWidth;

	std::vector<LocalAndGlobalThermostatVariables> _thermVars;

	double _dTargetTemperature;
	double _dTemperatureExponent;
	unsigned int _nTargetComponentID;
	unsigned short _nNumThermostatedTransDirections;

	unsigned short _nRegionID;

	Accumulator* _accumulator;

	std::string _strFilenamePrefixBetaLog;
	unsigned long _nWriteFreqBeta;
	unsigned long _numSampledConfigs;
	double _dBetaTransSumGlobal;
	double _dBetaRotSumGlobal;
	double _nuAndersen;
	double _timestep;
	double _nuDt;
	Random _rand;
};


class Domain;
class TemperatureControl
{
public:
	TemperatureControl();
	~TemperatureControl();

	void readXML(XMLfileUnits& xmlconfig);
	void AddRegion(ControlRegionT* region);
	int GetNumRegions() {return _vecControlRegions.size();}
	ControlRegionT* GetControlRegion(unsigned short nRegionID) {return _vecControlRegions.at(nRegionID-1); }  // vector index starts with 0, region index with 1

	void Init(unsigned long simstep);
	void MeasureKineticEnergy(Molecule* mol, DomainDecompBase* domainDecomp, unsigned long simstep);
	void CalcGlobalValues(DomainDecompBase* domainDecomp, unsigned long simstep);
	void ControlTemperature(Molecule* mol, unsigned long simstep);

	unsigned long GetStart() {return _nStart;}
	unsigned long GetStop()  {return _nStop;}

	// beta log file
	void InitBetaLogfiles();
	void WriteBetaLogfiles(unsigned long simstep);

	// loops over molecule container
	void DoLoopsOverMolecules(DomainDecompBase*, ParticleContainer* particleContainer, unsigned long simstep);
	void VelocityScalingPreparation(DomainDecompBase *, ParticleContainer *, unsigned long simstep);

private:
	std::vector<ControlRegionT*> _vecControlRegions;
	unsigned long _nControlFreq;
	unsigned long _nStart;
	unsigned long _nStop;

	enum ControlMethod {
		VelocityScaling,
		Andersen,
		Mixed
	};
	ControlMethod _method = VelocityScaling;
};


// Accumulate kinetic energy dependent on which translatoric directions should be thermostated

class Accumulator
{
private:
	bool _accumulateX, _accumulateY, _accumulateZ;

public:
	Accumulator(bool accX, bool accY, bool accZ) :
			_accumulateX(accX), _accumulateY(accY), _accumulateZ(accZ) {
	}

	double CalcKineticEnergyContribution(Molecule* mol) {
		double vx = _accumulateX ? mol->v(0) : 0.0;
		double vy = _accumulateY ? mol->v(1) : 0.0;
		double vz = _accumulateZ ? mol->v(2) : 0.0;
		double m  = mol->mass();

		return m * (vx*vx + vy*vy + vz*vz);
	}
	void ScaleVelocityComponents(Molecule* mol, double vcorr) {
		if (_accumulateX) mol->setv(0, mol->v(0) * vcorr);
		if (_accumulateY) mol->setv(1, mol->v(1) * vcorr);
		if (_accumulateZ) mol->setv(2, mol->v(2) * vcorr);
	}
};

#endif /* TEMPERATURECONTROL_H_ */
