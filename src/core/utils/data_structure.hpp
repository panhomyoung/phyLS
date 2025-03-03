#pragma once

#include <vector>
#include <unordered_map>

namespace mockturtle {
struct index_phase_pair {
  index_phase_pair() = default;
  index_phase_pair(uint32_t index, uint32_t phase) : node_index(index), node_phase(phase){}
  uint32_t node_index;
  uint8_t node_phase;

  std::array<uint64_t, 2> pins { 0, 0 };   /* pins recorded specificly for invertor */
  bool is_inverter { false };
};

struct index_cut_supergate {
  // infomation of the supergate
  uint32_t index;
  uint32_t cut_index;
  uint32_t supergate_index;
  uint8_t phase;

  // statistic of the supergate
  double area{0};
  double delay{0};
  double wirelength{0};
  double totalwirelength{0};

  struct CompareArea {
    bool operator()(index_cut_supergate const& lhs,
                    index_cut_supergate const& rhs) const {
      // Compare area
      if (lhs.area < rhs.area) return true;
      if (lhs.area > rhs.area) return false;

      // Compare delay
      if (lhs.delay < rhs.delay) return true;
      if (lhs.delay > rhs.delay) return false;

      // Compare index
      return lhs.index < rhs.index;
    }
  };

  struct CompareDelay {
    bool operator()(index_cut_supergate const& lhs,
                    index_cut_supergate const& rhs) const {
      if (lhs.delay < rhs.delay) return true;
      if (lhs.delay > rhs.delay) return false;

      if (lhs.area < rhs.area) return true;
      if (lhs.area > rhs.area) return false;

      return lhs.index <= rhs.index;
    }
  };

  struct CompareWirelength {
    bool operator()(index_cut_supergate const& lhs,
                    index_cut_supergate const& rhs) const {
      if (lhs.wirelength < rhs.wirelength) return true;
      if (lhs.wirelength > rhs.wirelength) return false;

      if (lhs.area < rhs.area) return true;
      if (lhs.area > rhs.area) return false;

      return lhs.index <= rhs.index;
    }
  };

  struct CompareTotalWirelength {
    bool operator()(index_cut_supergate const& lhs,
                    index_cut_supergate const& rhs) const {
      if (lhs.totalwirelength < rhs.totalwirelength) return true;
      if (lhs.totalwirelength > rhs.totalwirelength) return false;

      if (lhs.delay < rhs.delay) return true;
      if (lhs.delay > rhs.delay) return false;

      return lhs.index <= rhs.index;
    }
  };
};
} // namespace mockturtle

