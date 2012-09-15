/*
 * RDFForceIntegratorExtendedSite.cpp
 *
 *  Created on: Aug 30, 2012
 *      Author: tijana
 */

#include "RDFForceIntegratorExtendedSite.h"

bool RDFForceIntegratorExtendedSite::called = false;
double RDFForceIntegratorExtendedSite::_dr = 0;
double RDFForceIntegratorExtendedSite::_dn = 0;
double RDFForceIntegratorExtendedSite::_extension = 0;
double RDFForceIntegratorExtendedSite::_d_alpha = 0;
double RDFForceIntegratorExtendedSite::_d_level = 0;
int RDFForceIntegratorExtendedSite::_n_levels = 0;
int RDFForceIntegratorExtendedSite::_n_n = 0;
int RDFForceIntegratorExtendedSite::_n_r = 0;
int RDFForceIntegratorExtendedSite::_n_alpha = 0;
double* RDFForceIntegratorExtendedSite::_scaling_factors = NULL;

std::vector<std::vector<double> >
		RDFForceIntegratorExtendedSite::globalNondecliningDist = std::vector<
				std::vector<double> >();
std::vector<std::vector<double> >
		RDFForceIntegratorExtendedSite::globalNondecliningADist = std::vector<
				std::vector<double> >();
std::vector<std::vector<std::vector<double> > >
		RDFForceIntegratorExtendedSite::globalNondecliningSiteDist =
				std::vector<std::vector<std::vector<double> > >();
std::vector<std::vector<std::vector<double> > >
		RDFForceIntegratorExtendedSite::globalNondecliningSiteADist =
				std::vector<std::vector<std::vector<double> > >();
std::vector<double> RDFForceIntegratorExtendedSite::rmids =
		std::vector<double>();

RDFForceIntegratorExtendedSite::RDFForceIntegratorExtendedSite(
		ParticleContainer* moleculeContainer, double rc, std::vector<
				std::vector<double> >* globalADist, std::vector<std::vector<
				std::vector<double> > >* globalSiteADist) :
	RDFForceIntegrator(moleculeContainer, rc, globalADist, globalSiteADist) {
	// TODO Auto-generated constructor stub

}

RDFForceIntegratorExtendedSite::~RDFForceIntegratorExtendedSite() {
	// TODO Auto-generated destructor stub
}

void RDFForceIntegratorExtendedSite::precomputeScalingFactors() {
	// making sure this method is only called once since factors don't change
	// througout the simulation
	if (called)
		return;
	if (called == false)
		called = true;

	std::string
			rdf_file =
					"/home/tijana/Desktop/thesis/tijana/Ethan_10k_epsilon/prolonged/rdf_nondeclining/rdf_Ethan_10k_eps_double_prolonged_rc4_0-0.000090000.rdf";
	std::vector<std::string> file_names;
	file_names.push_back(rdf_file);

	// componentids that appear in the container, number of sites per such component
	std::vector<unsigned int> componentIds;
	std::vector<int> numSites;
	Molecule* currentMolecule;

	// find which component ids appear
	for (currentMolecule = _moleculeContainer->begin(); currentMolecule
			!= _moleculeContainer->end(); currentMolecule
			= _moleculeContainer->next()) {
		unsigned int i;
		for (i = 0; i < componentIds.size(); i++) {
			if (currentMolecule->componentid() == componentIds[i])
				break;
		}
		if (i == componentIds.size()) {
			componentIds.push_back(currentMolecule->componentid());
			numSites.push_back(currentMolecule->numSites());
		}
	}

	unsigned int totalComponents = componentIds.size();

	if (file_names.size() != totalComponents * totalComponents)
		std::cout << "Wrong number of rdf input files. There are "
				<< totalComponents * totalComponents
				<< " component combinations requiring the same number of rdf input files."
				<< std::endl;

	// read rdf files for different component pairs
	for (unsigned int i = 0; i < totalComponents; i++) {
		for (unsigned int j = 0; j < totalComponents; j++) {
			globalNondecliningDist.push_back(std::vector<double>());
			globalNondecliningADist.push_back(std::vector<double>());
			globalNondecliningSiteDist.push_back(std::vector<
					std::vector<double> >());
			globalNondecliningSiteADist.push_back(std::vector<std::vector<
					double> >());
			RDF::readRDFInputFile(file_names[i * totalComponents + j].c_str(),
					i, j, numSites[i], numSites[j], &rmids,
					&globalNondecliningDist[i * totalComponents + j],
					&globalNondecliningADist[i * totalComponents + j],
					&globalNondecliningSiteDist[i * totalComponents + j],
					&globalNondecliningSiteADist[i * totalComponents + j]);
		}
	}

	std::cout << "started precomputing scaling factors" << std::endl;

	_d_level = _dn; // levels spacing (level is the boundary level)
	_n_levels = (int) ((_rc + 2 * _extension) / _d_level + 0.5) + 1; // num levels
	_n_n = (int) (2 * _extension / _dn + 0.5) + 1; // number in normal direction
	_n_r = (int) ((_rc + 2 * _extension) / _dr + 0.5) + 1; // num radial direction
	_d_alpha = 10; // angle spacing (degrees)
	_n_alpha = (int) (360 / _d_alpha + 0.5); // num angles
	_scaling_factors = new double[_n_levels * _n_n * _n_r]; // array for the factors

	for (int i = 0; i < _n_levels * _n_n * _n_r; i++)
		_scaling_factors[i] = -10; //initialize to some nonsense value

	double above, below, curr_g, z, x, curr_z, curr_x, curr_y, alpha, scale,
			curr_r, mol_x, mol_y, mol_r, level, mol_z, phi;
	int curr_bin;

	for (int idx_level = 0; idx_level < _n_levels; idx_level++) {
		level = idx_level * _d_level;

		for (int idx_n = 0; idx_n < _n_n; idx_n++) {
			z = level - _extension + idx_n * _dn + _dn / 2; // site normal coord

			for (int idx_r = 0; idx_r < _n_r; idx_r++) {
				x = idx_r * _dr + _dr / 2; // site radial coord

				above = 0;
				below = 0;
				// probing the sphere
				for (int idx_alpha = 0; idx_alpha < _n_alpha; idx_alpha++) {
					alpha = idx_alpha * _d_alpha;

					curr_z = z + 2 * _extension * sin(alpha * PI / 180); // site of probing coord
					mol_z = z + _extension * sin(alpha * PI / 180); // coord of the center

					for (int idx_phi = 0; idx_phi < _n_alpha; idx_phi++) {
						phi = idx_phi * _d_alpha; // angle in plane

						// site coord
						curr_x = x + 2 * _extension * cos(phi * PI / 180)
								* cos(alpha * PI / 180);
						curr_y = 2 * _extension * sin(phi * PI / 180) * cos(
								alpha * PI / 180);

						// center coord
						mol_x = x + _extension * cos(phi * PI / 180) * cos(
								alpha * PI / 180);
						mol_y = _extension * sin(phi * PI / 180) * cos(alpha
								* PI / 180);

						// distance
						curr_r = std::sqrt(curr_x * curr_x + curr_z * curr_z
								+ curr_y * curr_y);
						mol_r = std::sqrt(mol_x * mol_x + mol_y * mol_y + mol_z
								* mol_z);

						curr_g = 1;

						if (curr_r < _rc + 2 * _extension) {

							curr_bin = (int) (curr_r
									* globalNondecliningSiteADist[0][0].size()
									/ (_rc + 2 * _extension) - 0.5);
							curr_g
									= globalNondecliningSiteADist[0][0][curr_bin];
							//std::cout<<"curr_bin "<<curr_bin<<" g "<<curr_g<<std::endl;
						}
						// if not in the circle of radius _rc + _extension above level
						if (mol_z < level) {
							below += curr_g;
						} else {
							above += curr_g;
						}

					}

				}

				if (above + below != 0)
					scale = above / (above + below);
				else
					scale = 0;

				//if (std::sqrt(x * x + z * z) >= _rc - 2 * _extension) {
				//	scale *= 0.8;
				//}
				_scaling_factors[idx_level * _n_n * _n_r + idx_n * _n_r + idx_r]
						= scale;
			}
		}
	}
	std::cout << "finished precomputing scaling factors" << std::endl;
}

double RDFForceIntegratorExtendedSite::getScalingFactor(int idx_level,
		int idx_n, int idx_r) {
	double above = 0;
	double below = 0;
	double level = idx_level * _d_level;
	double z = level - _extension + idx_n * _dn + _dn / 2;
	double x = idx_r * _dr + _dr / 2; // site radial coord
	double scale, curr_x, curr_y, curr_z, mol_x, mol_y, mol_z, mol_r, curr_r,
			alpha, phi, curr_g;
	int curr_bin;
	// probing the sphere
	for (int idx_alpha = 0; idx_alpha < _n_alpha; idx_alpha++) {
		alpha = idx_alpha * _d_alpha;

		curr_z = z + 2 * _extension * sin(alpha * PI / 180); // site of probing coord
		mol_z = z + _extension * sin(alpha * PI / 180); // coord of the center

		for (int idx_phi = 0; idx_phi < _n_alpha; idx_phi++) {
			phi = idx_phi * _d_alpha; // angle in plane

			// site coord
			curr_x = x + 2 * _extension * cos(phi * PI / 180) * cos(alpha * PI
					/ 180);
			curr_y = 2 * _extension * sin(phi * PI / 180) * cos(alpha * PI
					/ 180);

			// center coord
			mol_x = x + _extension * cos(phi * PI / 180)
					* cos(alpha * PI / 180);
			mol_y = _extension * sin(phi * PI / 180) * cos(alpha * PI / 180);

			// distance
			curr_r = std::sqrt(curr_x * curr_x + curr_z * curr_z + curr_y
					* curr_y);
			mol_r = std::sqrt(mol_x * mol_x + mol_y * mol_y + mol_z * mol_z);

			curr_g = 1;

			if (curr_r < _rc + 2 * _extension) {

				curr_bin = (int) (curr_r
						* globalNondecliningSiteADist[0][0].size() / (_rc + 2
						* _extension) - 0.5);
				curr_g = globalNondecliningSiteADist[0][0][curr_bin];
				//std::cout<<"curr_bin "<<curr_bin<<" g "<<curr_g<<std::endl;
			}
			// if not in the circle of radius _rc + _extension above level
			if (mol_z < level) {
				below += curr_g;
			} else {
				above += curr_g;
			}
		}

	}

	if (above + below != 0)
		scale = above / (above + below);
	else
		scale = 0;

	return scale;
}

void RDFForceIntegratorExtendedSite::integrateRDFSite(Molecule* mol,
		double* normal_dim, int* boundary, int plane, unsigned int site) {
	std::vector<double> globalAcc = (*_globalADist)[0];
	std::vector<std::vector<double> > globalSiteAcc = (*_globalSiteADist)[0];

	double V = (_rmax[0] - _rmin[0]) * (_rmax[1] - _rmin[1]) * (_rmax[2]
			- _rmin[2]);

	// number density of the domain
	//int numMolecules = _moleculeContainer->getNumberOfParticles();
	_numMolecules = 9826;
	double rho = _numMolecules / (V);

	double ref_area = 4 * _extension * _extension;
	double r, curr_x, curr_z, curr_g, curr_r, above, below, level, scale = 1;

	int bin, curr_bin; // bin for the radius that rdf is read for
	double g;
	for (double z = normal_dim[0] + _dn / 2; z <= normal_dim[1]; z += _dn) {
		for (double x = _dr / 2; x <= std::sqrt(normal_dim[1] * normal_dim[1]
				- z * z); x += _dr) {
			r = std::sqrt(x * x + z * z);
			level = normal_dim[0] + _extension; // boundary level


			// check if have to integrate over more than half sphere
			if (mol->r(plane) + mol->site_d(site)[plane] - _rmin[plane]
					< _extension)
				scale = 1;

			// check if needs scaling
			else if (z - normal_dim[0] < 2 * _extension) {

				int idx_r = (int) (x / _dr);
				int idx_level = (int) (level / _d_level + 0.5);
				int idx_n = (int) ((z - normal_dim[0]) / _dn);
				//std::cout<<"idxlevel: "<<idx_level<<" value "<< mol->r(plane) + mol->site_d(site)[plane] - _rmin[plane]<< "idx_n" <<idx_n<<" idx_r "<<idx_r<<std::endl;
				scale = //getScalingFactor(idx_level, idx_n, idx_r);
						_scaling_factors[idx_level * _n_n * _n_r + idx_n * _n_r
								+ idx_r];

			}

			if (mol->numSites() == 1) {
				bin = (int) (r * globalAcc.size() / (_rc + 2 * _extension)
						- 0.5);
				g = globalAcc[bin];
				g *= scale;

				double sig2 = mol->getSigma() * mol->getSigma();
				double r2 = r * r;
				double lj6 = sig2 * sig2 * sig2 / (r2 * r2 * r2);
				double ljf = 24 * mol->getEps() * (lj6 - 2 * lj6 * lj6) / r;

				double f[3] = { 0, 0, 0 };
				f[plane] = -2 * PI * rho * g * ljf * z * x * _dr * _dn / r;
				//mol->Fljcenteradd(site, f);

				if (boundary[0] == -1 && plane == 0) {
					mol->addLeftxRdfInfluence(site, f);
				}
			} else {
				bin = (int) (r * globalSiteAcc[0].size() / (_rc + 2
						* _extension) - 0.5);

				// if multiple LJ centers, use site-site rdf
				// iterate through sites, treat cell center as a site

				for (unsigned int j = 0; j < mol->numLJcenters(); j++) {

					/*if (z - normal_dim[0] < 2 * _extension && mol->r(plane)
					 + mol->site_d(site)[plane] - _rmin[plane]
					 >= _extension) {
					 g = globalNondecliningSiteADist[0][site
					 * mol->numLJcenters() + j][bin];
					 } else {
					 */
					g = globalSiteAcc[site * mol->numLJcenters() + j][bin];
					//}
					if (site != j)
						g /= 2;

					g *= scale;

					double sig2 = mol->getSigma() * mol->getSigma();
					double r2 = r * r;
					double lj6 = sig2 * sig2 * sig2 / (r2 * r2 * r2);

					double ljf = 24 * mol->getEps() * (lj6 - 2 * lj6 * lj6) / r;

					double f[3] = { 0, 0, 0 };
					f[plane] = -2 * PI * rho * g * ljf * z * x * _dr * _dn / r;
					//mol->Fljcenteradd(site, f);

					if (boundary[0] == -1 && plane == 0) {
						mol->addLeftxRdfInfluence(site, f);
					}
				}

			}
		}
	}
}

double RDFForceIntegratorExtendedSite::traverseMolecules() {
	Molecule* currentMolecule = _moleculeContainer->begin();
	_extension = currentMolecule->ljcenter_disp(0);
	// box size for the numerical quadrature
	_dn = _dr = currentMolecule->getSigma() / 20;
	// iterate through molecules and add rdf influence
	double total_pot = 0;
	precomputeScalingFactors();

	for (currentMolecule = _moleculeContainer->begin(); currentMolecule
			!= _moleculeContainer->end(); currentMolecule
			= _moleculeContainer->next()) {

		for (unsigned int site = 0; site < currentMolecule->numSites(); site++) {
			double rm[3] = { currentMolecule->r(0), currentMolecule->r(1),
					currentMolecule->r(2) };

			double r[3] = { currentMolecule->r(0) + currentMolecule->site_d(
					site)[0], currentMolecule->r(1) + currentMolecule->site_d(
					site)[1], currentMolecule->r(2) + currentMolecule->site_d(
					site)[2] };

			// if this is a halo molecule, skip it
			if (rm[0] < _rmin[0] || rm[1] < _rmin[1] || rm[2] < _rmin[2]
					|| rm[0] > _rmax[0] || rm[1] > _rmax[1] || rm[2] > _rmax[2])
				continue;

			// if the molecule is not within rc of the boundary, skip it
			if (rm[0] > _low_limit[0] && rm[0] < _high_limit[0] && rm[1]
					> _low_limit[1] && rm[1] < _high_limit[1] && rm[2]
					> _low_limit[2] && rm[2] < _high_limit[2])
				continue;

			if (currentMolecule->numSites() != currentMolecule->numLJcenters()) {
				std::cout
						<< "Molecule consists of something other than LJ centers. In this case RDF cannot be used.";
				return 0;
			}

			int boundary[3] = { 2, 2, 2 };
			if (rm[0] < _low_limit[0])
				boundary[0] = -1;
			else if (rm[0] >= _high_limit[0])
				boundary[0] = 1;
			if (rm[1] < _low_limit[1])
				boundary[1] = -1;
			else if (rm[1] >= _high_limit[1])
				boundary[1] = 1;
			if (rm[2] < _low_limit[2])
				boundary[2] = -1;
			else if (rm[2] >= _high_limit[2])
				boundary[2] = 1;

			//if (boundary[0] == 0 && boundary[1] == 0 && boundary[2] == 0)
			if (boundary[0] != -1)
				continue;

			// if molecule close to the boundary, add RDF force
			// integration limits for axes

			double normal_lim[2] = { 0, _rc + _extension + r[0] - rm[0]};

			if (boundary[0] == -1) {
				normal_lim[0] = std::abs(r[0] - _rmin[0] - _extension);
				if (r[0] < _rmin[0] + _extension)
					normal_lim[0] = std::abs(2 * r[0] - _rmin[0] - _extension);
			} else if (boundary[0] == 1) {
				normal_lim[0] = std::abs(_rmax[0] - _extension - r[0]);
			}
			//if (currentMolecule->r(0) > 10) std::cout<<"boundary: "<<boundary[0]<<" "<<boundary[1]<<" "<<boundary[2]<<std::endl;
			integrateRDFSite(currentMolecule, normal_lim, boundary, 0, site);

			// xy plane is the boundary
			// integrate xy
			/*
			 for (unsigned int i = 0; boundary[2] != 0 && i
			 < componentIds.size(); i++) {
			 if (boundary[2] == -1) {
			 zlim[0] = r2 - rc;
			 zlim[1] = (r2 > rmin[2] ? rmin[2] : 2 * r2 - rmin[2]);
			 normal_lim[0] = abs(r2 - rmin[2]);
			 } else if (boundary[2] == 1) {
			 zlim[0] = (r2 < rmax[2] ? rmax[2] : 2 * r2 - rmax[2]);
			 zlim[1] = r2 + rc;
			 normal_lim[0] = abs(rmax[2] - r2);
			 }
			 double h = zlim[1] - zlim[0];
			 double a = std::sqrt(h * (2 * rc - h));
			 if (boundary[0] == 0)
			 integrateRDFSite(normal_lim, currentMolecule, rc, dn, dr,
			 globalADist[currentMolecule->componentid()
			 * totalComponents + i],
			 globalSiteADist[currentMolecule->componentid()
			 * totalComponents + i], 2, currSum, site,
			 boundary);
			 else {
			 if (boundary[0] == -1) {
			 xlim[0] = rmin[0];
			 xlim[1] = r0 + a;
			 } else if (boundary[0] == 1) {
			 xlim[0] = r0 - a;
			 xlim[1] = rmax[0];
			 }

			 ylim[0] = r1 - a;
			 ylim[1] = r1 + a;
			 integrateRDFCartesian(xlim, ylim, zlim, currentMolecule,
			 rc, dx, dy, dz,
			 globalADist[currentMolecule->componentid()
			 * totalComponents + i],
			 globalSiteADist[currentMolecule->componentid()
			 * totalComponents + i], 2, currSum, site,
			 boundary);
			 }

			 }

			 // xz plane is the boundary
			 // integrate xz plane
			 for (unsigned int i = 0; boundary[1] != 0 && i
			 < componentIds.size(); i++) {
			 if (boundary[1] == -1) {
			 ylim[1] = (r1 > rmin[1] ? rmin[1] : 2 * r1 - rmin[1]);
			 ylim[0] = r1 - rc;
			 normal_lim[0] = abs(r1 - rmin[1]);
			 } else if (boundary[1] == 1) {
			 ylim[0] = (r1 < rmax[1] ? rmax[1] : 2 * r1 - rmax[1]);
			 ylim[1] = r1 + rc;
			 normal_lim[0] = abs(rmax[1] - r1);
			 }
			 double h = ylim[1] - ylim[0];
			 double a = std::sqrt(h * (2 * rc - h));
			 if (boundary[0] == -1) {
			 xlim[0] = rmin[0];
			 xlim[1] = r0 + a;
			 } else if (boundary[0] == 1) {
			 xlim[0] = r0 - a;
			 xlim[1] = rmax[0];
			 } else {
			 xlim[0] = r0 - a;
			 xlim[1] = r0 + a;
			 }
			 if (boundary[2] == -1) {
			 zlim[0] = rmin[2];
			 zlim[1] = r2 + rc;
			 } else if (boundary[2] == 1) {
			 zlim[0] = r2 - rc;
			 zlim[1] = rmax[2];
			 } else {
			 zlim[0] = r2 - a;
			 zlim[1] = r2 + a;
			 }
			 if (boundary[0] == 0 && boundary[2] == 0)
			 integrateRDFSite(normal_lim, currentMolecule, rc, dr, dn,
			 globalADist[currentMolecule->componentid()
			 * totalComponents + i],
			 globalSiteADist[currentMolecule->componentid()
			 * totalComponents + i], 1, currSum, site,
			 boundary);
			 else
			 integrateRDFCartesian(xlim, ylim, zlim, currentMolecule,
			 rc, dx, dy, dz,
			 globalADist[currentMolecule->componentid()
			 * totalComponents + i],
			 globalSiteADist[currentMolecule->componentid()
			 * totalComponents + i], 1, currSum, site,
			 boundary);
			 }

			 delete currSum;
			 */
		}
	}
	return total_pot;
}

