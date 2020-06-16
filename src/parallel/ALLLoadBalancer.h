/**
 * @file ALLLoadBalancer.h
 * @author seckler
 * @date 04.06.19
 */

#pragma once
#ifdef ENABLE_ALLLBL
#include <ALL.hpp>
#include "LoadBalancer.h"

class ALLLoadBalancer : public LoadBalancer {
public:
	ALLLoadBalancer(std::array<double, 3> boxMin, std::array<double, 3> boxMax, double gamma, MPI_Comm comm,
					std::array<size_t, 3> globalSize, std::array<size_t, 3> localCoordinates,
					double minimalPartitionSize);

	~ALLLoadBalancer() override = default;
	std::tuple<std::array<double, 3>, std::array<double, 3>> rebalance(double work) override;

private:
	ALL<double, double> _all;
	using Point = ALL_Point<double>;
	double _minimalPartitionSize{};
};
#endif