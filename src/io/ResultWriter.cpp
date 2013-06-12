/***************************************************************************
 *   Copyright (C) 2010 by Martin Bernreuther <bernreuther@hlrs.de> et al. *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "io/ResultWriter.h"

#include "particleContainer/ParticleContainer.h"
#include "parallel/DomainDecompBase.h"
#include "Domain.h"

#include <ctime>

using namespace std;

#define ACC_STEPS 1000

ResultWriter::ResultWriter(unsigned long writeFrequency, string outputPrefix)
: _writeFrequency(writeFrequency),
	_outputPrefix(outputPrefix),
	_U_pot_acc(ACC_STEPS),
	_p_acc(ACC_STEPS)
{ }

ResultWriter::~ResultWriter(){}

void ResultWriter::initOutput(ParticleContainer* particleContainer,
			      DomainDecompBase* domainDecomp, Domain* domain){
	 
	// initialize result file
	string resultfile(_outputPrefix+".res");
	time_t now;
	time(&now);
	if(domainDecomp->getRank()==0){
		_resultStream.open(resultfile.c_str());
		_resultStream << "# ls1 MarDyn simulation started at " << ctime(&now) << endl;
		_resultStream << "#step\tt\t\tU_pot\tU_pot_avg\t\tp\tp_avg\t\tbeta_trans\tbeta_rot\t\tc_v" << endl;
	}
}

void ResultWriter::doOutput( ParticleContainer* particleContainer, DomainDecompBase* domainDecomp, Domain* domain,
	unsigned long simstep, list<ChemicalPotential>* lmu )
{
	_U_pot_acc.addEntry(domain->getAverageGlobalUpot());
	_p_acc.addEntry(domain->getGlobalPressure());
	if((domainDecomp->getRank() == 0) && (simstep % _writeFrequency == 0)){
		_resultStream << simstep << "\t" << domain->getCurrentTime()
		              << "\t\t" << domain->getAverageGlobalUpot() << "\t" << _U_pot_acc.getAverage()
					  << "\t\t" << domain->getGlobalPressure() << "\t" << _p_acc.getAverage()
		              << "\t\t" << domain->getGlobalBetaTrans() << "\t" << domain->getGlobalBetaRot()
		              << "\t\t" << domain->cv() << "\n";
	}
}

void ResultWriter::finishOutput(ParticleContainer* particleContainer,
				DomainDecompBase* domainDecomp, Domain* domain){
	time_t now;
	time(&now);
	_resultStream << "# ls1 MarDyn simulation finished at " << ctime(&now) << endl;
	_resultStream.close();
}
