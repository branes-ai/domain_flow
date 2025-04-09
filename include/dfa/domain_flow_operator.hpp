// DomainFlowOperatorIterator.h
#pragma once

#include <iterator>
#include <type_traits>

namespace sw {
    namespace dfa {

        enum class DomainFlowOperator {
            FUNCTION_ARGUMENT,
            ABS,
            ADD,
            CAST,
            CLAMP,
            CONCAT,
            CONSTANT,
            CONV2D,
            CONV3D,
            DEPTHWISE_CONV2D,
            EXP,
            FC,
            GATHER,

            SUB,
            MUL,
            DIV,
            MATMUL,
            NEGATE,
            PAD,

            MAXPOOL2D,
            AVGPOOL2D,
            RECIPROCAL,
            REDUCE_ALL,
            REDUCE_MAX,
            REDUCE_MIN,
            REDUCE_SUM,
            REDUCE_PROD,

            RESHAPE,
            TRANSPOSE,
            TRANSPOSE_CONV2D,
            UNKNOWN,
            DomainFlowOperator_COUNT // Added to get the number of elements
        };

        // Output stream operator
        std::ostream& operator<<(std::ostream& os, DomainFlowOperator dfo) {
            switch (dfo) {
            case DomainFlowOperator::FUNCTION_ARGUMENT:   os << "FUNCTION_ARGUMENT";   break;
            case DomainFlowOperator::ABS:        os << "ABS";        break;
            case DomainFlowOperator::ADD:        os << "ADD";        break;
            case DomainFlowOperator::CAST:       os << "CAST";       break;
            case DomainFlowOperator::CLAMP:      os << "CLAMP";      break;
            case DomainFlowOperator::CONCAT:     os << "CONCAT";     break;
            case DomainFlowOperator::CONSTANT:   os << "CONSTANT";   break;
            case DomainFlowOperator::CONV2D:     os << "CONV2D";     break;
            case DomainFlowOperator::CONV3D:     os << "CONV3D";     break;
            case DomainFlowOperator::DEPTHWISE_CONV2D: os << "DEPTHWISE_CONV2D"; break;
            case DomainFlowOperator::FC:         os << "FC";         break;
            case DomainFlowOperator::EXP:        os << "EXP";        break;
            case DomainFlowOperator::GATHER:     os << "GATHER";     break;

            case DomainFlowOperator::SUB:        os << "SUB";        break;
            case DomainFlowOperator::MUL:        os << "MUL";        break;
            case DomainFlowOperator::DIV:        os << "DIV";        break;
            case DomainFlowOperator::MATMUL:     os << "MATMUL";     break;
            case DomainFlowOperator::NEGATE:     os << "NEGATE";     break;
            case DomainFlowOperator::PAD:        os << "PAD";        break;

            case DomainFlowOperator::MAXPOOL2D:  os << "MAXPOOL2D";  break;
            case DomainFlowOperator::AVGPOOL2D:  os << "AVGPOOL2D";  break;
            case DomainFlowOperator::RECIPROCAL: os << "RECIPROCAL"; break;

            case DomainFlowOperator::REDUCE_ALL: os << "REDUCE_ALL"; break;
            case DomainFlowOperator::REDUCE_MAX: os << "REDUCE_MAX"; break;
            case DomainFlowOperator::REDUCE_MIN: os << "REDUCE_MIN"; break;
            case DomainFlowOperator::REDUCE_SUM: os << "REDUCE_SUM"; break;
            case DomainFlowOperator::REDUCE_PROD: os << "REDUCE_PROD"; break;

            case DomainFlowOperator::RESHAPE:    os << "RESHAPE";    break;
            case DomainFlowOperator::TRANSPOSE:  os << "TRANSPOSE";  break;
            case DomainFlowOperator::TRANSPOSE_CONV2D: os << "TRANSPOSE_CONV2D"; break;
            case DomainFlowOperator::UNKNOWN:    os << "UNKNOWN";    break;
            default: throw std::invalid_argument("Unknown DomainFlowOperator value");
            }
            return os;
        }

        // Input stream operator
        std::istream& operator>>(std::istream& is, DomainFlowOperator& dfo) {
            std::string token;
            if (!std::getline(is, token, '|')) {  // Assuming '|' as delimiter from previous format
                is.setstate(std::ios::failbit);
                return is;
            }

            if (token == "FUNCTION_ARGUMENT")    dfo = DomainFlowOperator::FUNCTION_ARGUMENT;
            else if (token == "ABS")    dfo = DomainFlowOperator::ABS;
            else if (token == "ADD")    dfo = DomainFlowOperator::ADD;
            else if (token == "CAST")    dfo = DomainFlowOperator::CAST;
            else if (token == "CLAMP")    dfo = DomainFlowOperator::CLAMP;
            else if (token == "CONCAT") dfo = DomainFlowOperator::CONCAT;
            else if (token == "CONSTANT")    dfo = DomainFlowOperator::CONSTANT;
            else if (token == "CONV2D")    dfo = DomainFlowOperator::CONV2D;
            else if (token == "CONV3D")    dfo = DomainFlowOperator::CONV3D;
            else if (token == "DEPTHWISE_CONV2D") dfo = DomainFlowOperator::DEPTHWISE_CONV2D;
            else if (token == "FC")     dfo = DomainFlowOperator::FC;
            else if (token == "EXP")    dfo = DomainFlowOperator::EXP;
            else if (token == "GATHER") dfo = DomainFlowOperator::GATHER;

            else if (token == "SUB")    dfo = DomainFlowOperator::SUB;
            else if (token == "MUL")    dfo = DomainFlowOperator::MUL;
            else if (token == "DIV")    dfo = DomainFlowOperator::DIV;
            else if (token == "MATMUL") dfo = DomainFlowOperator::MATMUL;
            else if (token == "NEGATE") dfo = DomainFlowOperator::NEGATE;
            else if (token == "PAD")    dfo = DomainFlowOperator::PAD;

            else if (token == "MAXPOOL2D") dfo = DomainFlowOperator::MAXPOOL2D;
            else if (token == "AVGPOOL2D") dfo = DomainFlowOperator::AVGPOOL2D;
            else if (token == "RECIPROCAL") dfo = DomainFlowOperator::RECIPROCAL;

            else if (token == "REDUCE_ALL") dfo = DomainFlowOperator::REDUCE_ALL;
            else if (token == "REDUCE_MAX") dfo = DomainFlowOperator::REDUCE_MAX;
            else if (token == "REDUCE_MIN") dfo = DomainFlowOperator::REDUCE_MIN;
            else if (token == "REDUCE_SUM") dfo = DomainFlowOperator::REDUCE_SUM;
            else if (token == "REDUCE_PROD") dfo = DomainFlowOperator::REDUCE_PROD;

            else if (token == "RESHAPE") dfo = DomainFlowOperator::RESHAPE;
            else if (token == "TRANSPOSE") dfo = DomainFlowOperator::TRANSPOSE;
            else if (token == "TRANSPOSE_CONV2D") dfo = DomainFlowOperator::TRANSPOSE_CONV2D;
            else if (token == "UNKNOWN")   dfo = DomainFlowOperator::UNKNOWN;
            else {
                is.setstate(std::ios::failbit);
                throw std::invalid_argument("Invalid DomainFlowOperator string: " + token);
            }

            return is;
        }

        // To make the enum set an iteratable we need
        //  1. An iterator class that can increment through the enum values
        //  2. A range class that defines the begin and end points(from 0 to COUNT)
        //  3. A helper function for cleaner syntax when using the range
        // 
		// This provides the functionality to use this pattern to work with the enum:
		//  for (auto op : AllDomainFlowOperators()) {
        //      ... stuff with the op
		//  }

		// helper class to iterate over DomainFlowOperator enum values
        class DomainFlowOperatorIterator {
        private:
            using enum_type = std::underlying_type_t<DomainFlowOperator>;
            enum_type value;

        public:
            // Iterator traits
            using difference_type = std::ptrdiff_t;
            using value_type = DomainFlowOperator;
            using pointer = const DomainFlowOperator*;
            using reference = const DomainFlowOperator&;
            using iterator_category = std::input_iterator_tag;

            // Constructor
            explicit DomainFlowOperatorIterator(enum_type v) : value(v) {}

            // Dereference operator
            DomainFlowOperator operator*() const {
                return static_cast<DomainFlowOperator>(value);
            }

            // Pre-increment operator
            DomainFlowOperatorIterator& operator++() {
                ++value;
                return *this;
            }

            // Post-increment operator
            DomainFlowOperatorIterator operator++(int) {
                DomainFlowOperatorIterator temp = *this;
                ++(*this);
                return temp;
            }

            // Equality operators
            bool operator==(const DomainFlowOperatorIterator& other) const {
                return value == other.value;
            }

            bool operator!=(const DomainFlowOperatorIterator& other) const {
                return value != other.value;
            }
        };

        // Range class for DomainFlowOperator
        class DomainFlowOperatorRange {
        public:
            DomainFlowOperatorRange() {}

            DomainFlowOperatorIterator begin() const {
                return DomainFlowOperatorIterator(0);
            }

            DomainFlowOperatorIterator end() const {
                return DomainFlowOperatorIterator(
                    static_cast<std::underlying_type_t<DomainFlowOperator>>(
                        DomainFlowOperator::DomainFlowOperator_COUNT));
            }
        };

        // Helper function to get all operators as a range
        inline DomainFlowOperatorRange AllDomainFlowOperators() {
            return DomainFlowOperatorRange();
        }
    }
}