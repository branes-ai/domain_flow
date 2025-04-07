#pragma once

#include <graph/graph.hpp>

namespace sw {
    namespace dfa {

        // the Domain Flow Graph edge type
        struct DomainFlowEdge : public sw::graph::weighted_edge<int> { // Weighted by the data flow on this link
            int flow;
            bool stationair;  // does the flow go through a memory or not
            std::string shape;  // tensor<1x2x3x4x5> as example
			int scalarSizeInBits; // size of the scalar type of the tensor in bits
            std::vector<int> schedule;  // N-D vector of the schedule

            int weight() const noexcept override { return flow; }

            void setStationarity(bool inMemory) { stationair = inMemory; }
            void setShape(std::string shape) { this->shape = shape; }
            void setSchedule(std::vector<int> schedule) { this->schedule = schedule; }

            DomainFlowEdge() : flow{ 0 }, stationair{ true }, shape{ "1xi32" }, scalarSizeInBits{ 32 }, schedule{ {0,0,0} } {}
            DomainFlowEdge(int flow, bool inMemory = true) : flow{ flow }, stationair{ inMemory }, shape{ "1xi32" }, scalarSizeInBits{ 32 }, schedule{ {0,0,0} } {}
			DomainFlowEdge(int flow, bool inMemory, std::string shape, int scalarSizeInBits, std::vector<int> tau) : flow{ flow }, stationair{ inMemory }, shape{ shape }, scalarSizeInBits{ scalarSizeInBits }, schedule{ tau } {}
            ~DomainFlowEdge() {}
        };

        // Output stream operator
        std::ostream& operator<<(std::ostream& os, const DomainFlowEdge& df) {
            // Format: flow|stationair|shape|tau1,tau2,...
            os << df.flow << "|" << (df.stationair ? "true" : "false") << "|" << df.shape << "|" << df.scalarSizeInBits << "|";

            // schedule
            bool first = true;
            for (const auto& sched : df.schedule) {
                if (!first) os << ",";
                os << sched;
                first = false;
            }

            return os;
        }

        // Input stream operator
        std::istream& operator>>(std::istream& is, DomainFlowEdge& df) {
            std::string line;
            if (!std::getline(is, line)) {
                is.setstate(std::ios::failbit);
                return is;
            }

            std::istringstream iss(line);
            std::string segment;

            // flow (weight)
            if (!std::getline(iss, segment, '|')) {
                is.setstate(std::ios::failbit);
                return is;
            }
            std::istringstream(segment) >> df.flow;

            // stationair
            if (!std::getline(iss, segment, '|')) {
                is.setstate(std::ios::failbit);
                return is;
            }
            df.stationair = (segment == "true");

            // shape
            if (!std::getline(iss, df.shape, '|')) {
                is.setstate(std::ios::failbit);
                return is;
            }

			// scalarSizeInBits
			if (!std::getline(iss, segment, '|')) {
				is.setstate(std::ios::failbit);
				return is;
			}
			std::istringstream(segment) >> df.scalarSizeInBits;

            // scheduling vector
            df.schedule.clear();
            if (!std::getline(iss, segment)) {  // Last field, no delimiter at end
                is.setstate(std::ios::failbit);
                return is;
            }
            if (!segment.empty()) {
                std::istringstream sched_ss(segment);
                std::string sched_val;
                while (std::getline(sched_ss, sched_val, ',')) {
                    int val;
                    std::istringstream(sched_val) >> val;
                    df.schedule.push_back(val);
                }
            }

            return is;
        }

    }
}

